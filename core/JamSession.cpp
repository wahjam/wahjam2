// SPDX-License-Identifier: Apache-2.0
#include "JamSession.h"

JamSession::JamSession(AppView *appView_, QObject *parent)
    : QObject{parent}, appView{appView_}, state_{JamSession::Unconnected},
      metronome{appView}
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
            &metronome, &Metronome::processAudioStreams);
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
    return metronome.nextIntervalTime();
}

SampleTime JamSession::remainingIntervalTime(SampleTime pos) const
{
    return metronome.remainingIntervalTime(pos);
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

void JamSession::abort()
{
    if (state_ == JamSession::Unconnected) {
        return;
    }

    conn.abort();
    remoteIntervals.clear();
    deleteRemoteUsers();
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

    metronome.stop();
    setState(JamSession::Closing);
    conn.disconnectFromServer();
}

void JamSession::connConnected()
{
    setState(JamSession::Connected);
}

void JamSession::connDisconnected()
{
    metronome.stop();
    server_.clear();
    remoteIntervals.clear();
    deleteRemoteUsers();
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
    metronome.setNextBpmBpi(bpm, bpi);
    metronome.start();
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

        // TODO subscribe to channel?
        // TODO connect processAudioStreams
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

    // TODO how is silence handled?
    auto remoteInterval =
        std::make_shared<RemoteInterval>(username, guid, fourCC);

    RemoteUser *remoteUser = remoteUsers[username];
    if (!remoteUser->enqueueRemoteInterval(channelIndex, remoteInterval)) {
        return; // invalid channel index, throw away this interval
    }

    remoteIntervals.insert(guid, remoteInterval);
}

void JamSession::connDownloadIntervalReceived(const QUuid &guid,
                                              const QByteArray &data,
                                              bool last)
{
    if (!remoteIntervals.contains(guid)) {
        return;
    }

    auto remoteInterval = remoteIntervals[guid];
    remoteInterval->appendData(data);
    if (last) {
        remoteInterval->finishAppendingData();
        remoteIntervals.remove(guid);
    }
    // TODO how do silent intervals remove themselves from remoteIntervals?
}
