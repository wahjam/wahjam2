// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "AppView.h"
#include "IIntervalTime.h"
#include "JamConnection.h"
#include "Metronome.h"
#include "RemoteUser.h"

/*
 * JamSession implements a running jam session, including responding to
 * protocol messages and dealing with audio data. The user interface uses this
 * class as its interface for managing the currently active jam session.
 */
class JamSession : public QObject, public IIntervalTime
{
    Q_OBJECT
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString server READ server)
    Q_PROPERTY(QString topic READ server NOTIFY topicChanged)

public:
    enum State {
        Unconnected,
        Connecting,
        Connected,
        Closing,
    };
    Q_ENUM(State)

    JamSession(AppView *appView, QObject *parent = nullptr);
    ~JamSession();

    State state() const;
    QString server() const;
    QString topic() const;

    // Connect to a server, aborting any previous connection first. The state
    // will change to Connecting.
    Q_INVOKABLE
    void connectToServer(const QString &server,
                         const QString &username,
                         const QString &hexToken);

    // Disconnect or stop connection establishment. The state will change to
    // Closing first and then Unconnected once the connection has been
    // disconnected.
    Q_INVOKABLE
    void disconnectFromServer();

    // Send a public message to the jam session
    Q_INVOKABLE
    void sendChatMessage(const QString &msg);

    // Send a private message to a specific user
    Q_INVOKABLE
    void sendChatPrivMsg(const QString &username, const QString &msg);

    // TODO:
    // Remote channels
    // Local channels
    // BPM/BPI

    SampleTime nextIntervalTime() const;
    SampleTime remainingIntervalTime(SampleTime pos) const;

signals:
    void stateChanged(State newState);

    // For error reporting, stateChanged() is emitted for actual state change
    void error(const QString &msg);

    // The new topic and who changed it (who is empty if set by server)
    void topicChanged(const QString &who, const QString &newTopic);

    // Public chat message notification
    void chatMessageReceived(const QString &senderUsername,
                             const QString &msg);

    // Private chat message notification
    void chatPrivMsgReceived(const QString &senderUsername,
                             const QString &msg);

    // Server message notification
    void chatServerMsgReceived(const QString &msg);

private:
    AppView *appView;
    JamConnection conn;
    State state_;
    QString server_;
    QString topic_;
    Metronome metronome;
    QHash<QString, RemoteUser*> remoteUsers;

    // Remote intervals with downloads in progress
    QHash<QUuid, std::shared_ptr<RemoteInterval> > remoteIntervals;

    void deleteRemoteUsers();

    void setState(State newState);

    // Disconnect immediately
    void abort();

private slots:
    void connConnected();
    void connDisconnected();
    void connError(const QString &errorString);
    void connConfigChanged(int bpm, int bpi);
    void connUserInfoChanged(const QList<JamConnection::UserInfo> &changes);
    void connDownloadIntervalBegan(const QUuid &guid,
                                   quint32 estimatedSize,
                                   const JamConnection::FourCC fourCC,
                                   quint8 channelIndex,
                                   const QString &username);
    void connDownloadIntervalReceived(const QUuid &guid,
                                      const QByteArray &data,
                                      bool last);
    void connChatMessageReceived(const QString &command,
                                 const QString &arg1,
                                 const QString &arg2,
                                 const QString &arg3,
                                 const QString &arg4);
};
