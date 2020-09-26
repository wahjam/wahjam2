// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <QTcpSocket>
#include <QTimer>

/*
 * JamConnection implements the network protocol for online jamming.  It
 * handles connection housekeeping internally and exposes the interesting
 * protocol messages to the user.
 */
class JamConnection : public QObject
{
    Q_OBJECT

public:
    typedef quint8 Guid[16];
    typedef quint8 FourCC[4];

    // Used to notify server of channels with sendChannelInfo()
    struct ChannelInfo
    {
        QString name;
        qint16 volume;
        qint8 pan;
        quint8 flags;
    };

    // Used to report remote user channels by userInfoChanged()
    struct UserInfo
    {
        quint8 active;
        quint8 channelIndex;
        qint16 volume;
        qint8 pan;
        quint8 flags;
        QString username;
        QString channelName;
    };

    JamConnection(QObject *parent = nullptr);
    ~JamConnection();

    void connectToServer(const QString &server,
                         const QString &username,
                         const QString &hexToken);

    // Fetch a human-readable error string after unexpected disconnect
    QString errorString() const;

    // Mute/unmute another user's channels
    bool sendSetUsermask(const QString &username,
                         const quint32 *mask,
                         size_t maskSize);

    bool sendChannelInfo(const QList<ChannelInfo> &channels);

    bool sendUploadIntervalBegin(const Guid guid,
                                 quint32 estimatedSize,
                                 const FourCC fourCC,
                                 quint8 channelIndex);

    bool sendUploadIntervalWrite(const Guid guid,
                                 quint8 flags,
                                 const quint8 *data,
                                 qint64 len);

    bool sendChatMessage(const QString &command,
                         const QString &arg1 = QString(),
                         const QString &arg2 = QString(),
                         const QString &arg3 = QString(),
                         const QString &arg4 = QString());

public slots:
    void disconnectFromServer();

signals:
    // Emitted when the connection started with connect() is established
    void connected();

    // Emitted when an established connection has closed
    void disconnected();

    // Emitted when an expected error occurs.  The connection is unusable.
    void error(const QString &errorString);

    // Prepare to change BPM and BPI next interval
    void configChanged(int bpm, int bpi);

    void userInfoChanged(const QList<UserInfo> &changes);

    void downloadIntervalBegan(const Guid guid,
                               quint32 estimatedSize,
                               const FourCC fourCC,
                               quint8 channelIndex,
                               const QString &username);

    void downloadIntervalReceived(const Guid guid, const QByteArray &data);

    void chatMessageReceived(const QString &command,
                             const QString &arg1,
                             const QString &arg2,
                             const QString &arg3,
                             const QString &arg4);

private slots:
    void socketConnected();
    void socketDisconnected();
    void socketError(QAbstractSocket::SocketError);
    void socketReadyRead();
    void sendKeepalive();
    void keepaliveExpired();

private:
    QString username_;
    QString hexToken_;
    QTimer sendKeepaliveTimer;      // when we need to send keepalives
    QTimer receiveKeepaliveTimer;   // when the other side didn't send keepalives
    QTcpSocket socket;
    QString error_;
    qint64 payloadSize;
    quint8 maxChannels_;

    void fail(const QString &errorString);
    void stopKeepaliveTimers();

    bool parseMessageHeader();
    bool parseAuthChallenge();
    bool parseAuthReply();
    bool parseConfigChangeNotify();
    bool parseUserInfoChangeNotify();
    bool parseDownloadIntervalBegin();
    bool parseDownloadIntervalWrite();
    bool parseChatMessage();
    bool parseKeepalive();
    bool parseMessage();

    bool send(quint8 type, const char *data, size_t len);
    bool send(quint8 type, const QByteArray &bytes);
    bool sendAuthUser(quint32 protocolVersion, const quint8 challenge[8]);
};
