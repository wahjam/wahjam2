// SPDX-License-Identifier: Apache-2.0
#include "JamSession.h"

JamSession::JamSession(AppView *appView, QObject *parent)
    : QObject{parent}, state_{JamSession::Unconnected},
      metronome{appView}
{
    connect(&conn, &JamConnection::connected,
            this, &JamSession::connConnected);
    connect(&conn, &JamConnection::disconnected,
            this, &JamSession::connDisconnected);
    connect(&conn, &JamConnection::error,
            this, &JamSession::connError);
    connect(&conn, &JamConnection::chatMessageReceived,
            this, &JamSession::connChatMessageReceived);
    connect(&conn, &JamConnection::configChanged,
            this, &JamSession::connConfigChanged);

    connect(appView, &AppView::processAudioStreams,
            &metronome, &Metronome::processAudioStreams);
}

JamSession::~JamSession()
{
    abort();
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
