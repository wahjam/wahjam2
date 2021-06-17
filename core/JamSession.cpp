// SPDX-License-Identifier: Apache-2.0
#include "JamSession.h"

JamSession::JamSession(AppView *appView_, QObject *parent)
    : QObject{parent}, appView{appView_}, state_{JamSession::Unconnected},
      metronome_{appView}
{
    connect(&conn, &JamConnection::connected,
            this, &JamSession::connConnected);
    connect(&conn, &JamConnection::disconnected,
            this, &JamSession::connDisconnected);
    connect(&conn, &JamConnection::error,
            this, &JamSession::connError);
    connect(&conn, &JamConnection::configChanged,
            this, &JamSession::connConfigChanged);
    connect(&conn, &JamConnection::userInfoChanged,
            this, &JamSession::connUserInfoChanged);
    connect(&conn, &JamConnection::downloadIntervalBegan,
            this, &JamSession::connDownloadIntervalBegan);
    connect(&conn, &JamConnection::downloadIntervalReceived,
            this, &JamSession::connDownloadIntervalReceived);
    connect(&conn, &JamConnection::chatMessageReceived,
            this, &JamSession::connChatMessageReceived);

    connect(appView, &AppView::processAudioStreams,
            &metronome_, &Metronome::processAudioStreams);
}

// Local channels currently only live while the jam session is connected.
// addLocalChannels() is called when the jam session starts and
// deleteLocalChannels() is called when it disconnects. This could be changed
// in the future to make the local channels persist across jam session so the
// user can still monitor and adjust audio outside a jam session.
void JamSession::addLocalChannels()
{
    if (!localChannels_.isEmpty()) {
        return;
    }

    AudioProcessor *processor = appView->audioProcessor();
    LocalChannel *chan = new LocalChannel{
        "channel0",
        0,
        &processor->captureStream(CHANNEL_LEFT),
        &processor->captureStream(CHANNEL_RIGHT),
        processor->getSampleRate(),
        this
    };
    connect(appView, &AppView::processAudioStreams,
            chan, &LocalChannel::processAudioStreams);
    connect(chan, &LocalChannel::uploadData,
            this, &JamSession::uploadData);
    localChannels_.push_back(chan);

    conn.sendChannelInfo({{chan->name(), 0, 0, 0}});

    chan->start();

    emit localChannelsChanged();
}

void JamSession::deleteLocalChannels()
{
    for (auto chan : localChannels_) {
        chan->deleteLater();
    }

    localChannels_.clear();
    emit localChannelsChanged();
}

const QVector<LocalChannel*> JamSession::localChannels() const
{
    return localChannels_;
}

void JamSession::deleteRemoteUsers()
{
    for (auto remoteUser : qAsConst(remoteUsers)) {
        delete remoteUser;
    }
    remoteUsers.clear();
}

JamSession::~JamSession()
{
    abort();
}

SampleTime JamSession::nextIntervalTime() const
{
    return metronome_.nextIntervalTime();
}

size_t JamSession::remainingIntervalTime(SampleTime pos) const
{
    return metronome_.remainingIntervalTime(pos);
}

JamSession::State JamSession::state() const
{
    return state_;
}

QString JamSession::server() const
{
    return server_;
}

QString JamSession::topic() const
{
    return topic_;
}

void JamSession::setState(State newState)
{
    state_ = newState;
    emit stateChanged(state_);
}

Metronome *JamSession::metronome()
{
    return &metronome_;
}

void JamSession::abort()
{
    if (state_ == JamSession::Unconnected) {
        return;
    }

    conn.abort();
    remoteIntervals.clear();
    deleteRemoteUsers();
    deleteLocalChannels();
}

void JamSession::connectToServer(const QString &server,
                                 const QString &username,
                                 const QString &hexToken)
{
    abort();

    qDebug("Connecting to %s as user \"%s\"...",
           server.toLatin1().constData(),
           username.toLatin1().constData());

    server_ = server;
    setState(JamSession::Connecting);
    conn.connectToServer(server, username, hexToken);
}

void JamSession::disconnectFromServer()
{
    if (state_ == JamSession::Unconnected) {
        return;
    }

    qDebug("Disconnecting from server %s...",
           server_.toLatin1().constData());

    metronome_.stop();
    deleteLocalChannels();
    setState(JamSession::Closing);
    conn.disconnectFromServer();
}

void JamSession::connConnected()
{
    setState(JamSession::Connected);
}

void JamSession::connDisconnected()
{
    metronome_.stop();
    server_.clear();
    remoteIntervals.clear();
    deleteRemoteUsers();
    deleteLocalChannels();
    setState(JamSession::Unconnected);
}

void JamSession::connError(const QString &errorString)
{
    emit error(errorString);
}

void JamSession::connChatMessageReceived(const QString &command,
                                         const QString &arg1,
                                         const QString &arg2,
                                         const QString &arg3,
                                         const QString &arg4)
{
    qDebug("Chat message received: \"%s\" \"%s\" \"%s\"",
           command.toLatin1().constData(),
           arg1.toLatin1().constData(),
           arg2.toLatin1().constData());

    if (command == "MSG") {
        if (arg1.isEmpty()) {
            emit chatServerMsgReceived(arg2);
        } else {
            emit chatMessageReceived(arg1, arg2);
        }
    } else if (command == "PRIVMSG") {
        emit chatPrivMsgReceived(arg1, arg2);
    } else if (command == "TOPIC") {
        auto who = arg1;
        topic_ = arg2;
        emit topicChanged(who, topic_);
    }
}

void JamSession::connConfigChanged(int bpm, int bpi)
{
    qDebug("Server config changed bpm=%d bpi=%d", bpm, bpi);
    metronome_.setNextBpmBpi(bpm, bpi);
    metronome_.start();
    addLocalChannels();
}

void JamSession::sendChatMessage(const QString &msg)
{
    conn.sendChatMessage("MSG", msg);
}

void JamSession::sendChatPrivMsg(const QString &username, const QString &msg)
{
    conn.sendChatMessage("PRIVMSG", username, msg);
}

void JamSession::connUserInfoChanged(const QList<JamConnection::UserInfo> &changes)
{
    for (auto userInfo : changes) {
        if (!remoteUsers.contains(userInfo.username)) {
            remoteUsers[userInfo.username] =
                new RemoteUser{userInfo.username, appView};

            /* Subscribe to all channels */
            conn.sendSetUsermask(userInfo.username, 0xffffffff);
        }

        qDebug("%s user \"%s\" channel \"%s\" (%d)",
               userInfo.active ? "Adding" : "Removing",
               userInfo.username.toLatin1().constData(),
               userInfo.channelName.toLatin1().constData(),
               userInfo.channelIndex);

        // Note that clients don't send volume and pan so the fields are unused
        RemoteUser *remoteUser = remoteUsers[userInfo.username];
        remoteUser->setChannelInfo(userInfo.channelIndex,
                                   userInfo.channelName,
                                   userInfo.active);
    }

    // Delete remote users with no channels
    QVector<QString> usersToRemove;
    for (auto i = remoteUsers.constKeyValueBegin();
         i != remoteUsers.constKeyValueEnd();
         ++i) {
        RemoteUser *remoteUser = (*i).second;
        if (remoteUser->numActiveChannels() == 0) {
            usersToRemove.append((*i).first);
        }
    }
    while (!usersToRemove.isEmpty()) {
        auto username = usersToRemove.takeLast();
        qDebug("Deleting user \"%s\"", username.toLatin1().constData());
        delete remoteUsers.take(username);

        QVector<QUuid> intervalsToRemove;
        for (auto remoteInterval : qAsConst(remoteIntervals)) {
            if (remoteInterval->username() == username) {
                intervalsToRemove.append(remoteInterval->guid());
            }
        }
        while (!intervalsToRemove.isEmpty()) {
            auto guid = intervalsToRemove.takeLast();
            remoteIntervals.remove(guid);
        }
    }
}

void JamSession::connDownloadIntervalBegan(const QUuid &guid,
                                           quint32 estimatedSize,
                                           const JamConnection::FourCC fourCC,
                                           quint8 channelIndex,
                                           const QString &username)
{
    if (!remoteUsers.contains(username)) {
        qWarning("Ignoring download interval for unknown user \"%s\"",
                 username.toLatin1().constData());
        return;
    }

    auto remoteInterval =
        std::make_shared<RemoteInterval>(username, guid, fourCC);

    RemoteUser *remoteUser = remoteUsers[username];
    if (!remoteUser->enqueueRemoteInterval(channelIndex, remoteInterval)) {
        return; // invalid channel index, throw away this interval
    }

    // Silence intervals have a zero GUID. Don't add them to the hash map
    // because the server will not send further messages related to them,
    // we won't known when to delete them. Instead drop our shared pointer
    // in this reference and let remoteUser decide how long the interval stays
    // alive.
    if (!remoteInterval->isSilence()) {
        remoteIntervals.insert(guid, remoteInterval);
    }
}

void JamSession::connDownloadIntervalReceived(const QUuid &guid,
                                              const QByteArray &data,
                                              bool last)
{
    if (!remoteIntervals.contains(guid)) {
        qDebug("Ignoring download interval received for unknown guid %s",
               guid.toString().toLatin1().constData());
        return;
    }

    auto remoteInterval = remoteIntervals[guid];
    remoteInterval->appendData(data);
    if (last) {
        remoteInterval->finishAppendingData();
        remoteIntervals.remove(guid);
    }
}

void JamSession::uploadData(int channelIdx, const QUuid &guid,
                            const QByteArray &data, bool first, bool last)
{
    if (first) {
        JamConnection::FourCC fourCC{'O', 'G', 'G', 'v'};
        conn.sendUploadIntervalBegin(guid, 0, fourCC, channelIdx);
    }
    if (!guid.isNull()) {
        conn.sendUploadIntervalWrite(
            guid,
            last ? 0x1 : 0x0,
            reinterpret_cast<const quint8*>(data.constData()),
            data.size()
        );
    }
}
