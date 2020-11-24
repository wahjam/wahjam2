// SPDX-License-Identifier: Apache-2.0
#include <QtEndian>
#include <QCryptographicHash>
#include "JamConnection.h"

// Annotate accesses to 8-bit fields - they don't need byteswapping
#define noEndian8Bit(x) (x)

enum {
    MSG_TYPE_SERVER_AUTH_CHALLENGE          = 0x0,
    MSG_TYPE_SERVER_AUTH_REPLY              = 0x1,
    MSG_TYPE_SERVER_CONFIG_CHANGE_NOTIFY    = 0x2,
    MSG_TYPE_SERVER_USERINFO_CHANGE_NOTIFY  = 0x3,
    MSG_TYPE_SERVER_DOWNLOAD_INTERVAL_BEGIN = 0x4,
    MSG_TYPE_SERVER_DOWNLOAD_INTERVAL_WRITE = 0x5,
    MSG_TYPE_CLIENT_AUTH_USER               = 0x80,
    MSG_TYPE_CLIENT_SET_USERMASK            = 0x81,
    MSG_TYPE_CLIENT_SET_CHANNEL_INFO        = 0x82,
    MSG_TYPE_CLIENT_UPLOAD_INTERVAL_BEGIN   = 0x83,
    MSG_TYPE_CLIENT_UPLOAD_INTERVAL_WRITE   = 0x84,
    MSG_TYPE_CHAT_MESSAGE                   = 0xc0,
    MSG_TYPE_KEEPALIVE                      = 0xfd,
};

struct MessageHeader
{
    quint8 type;
    quint32 length; // bytes, not including this header
} Q_PACKED;

JamConnection::JamConnection(QObject *parent)
    : QObject{parent}, payloadSize{0}, maxChannels{0}
{
    sendKeepaliveTimer.setSingleShot(true);
    receiveKeepaliveTimer.setSingleShot(true);
    connect(&sendKeepaliveTimer, &QTimer::timeout,
            this, &JamConnection::sendKeepalive);
    connect(&receiveKeepaliveTimer, &QTimer::timeout,
            this, &JamConnection::keepaliveExpired);

    connect(&socket, &QTcpSocket::connected,
            this, &JamConnection::socketConnected);
    connect(&socket, &QTcpSocket::disconnected,
            this, &JamConnection::socketDisconnected);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
    connect(&socket, &QAbstractSocket::errorOccurred,
            this, &JamConnection::socketError),
#else
    connect(&socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error),
            this, &JamConnection::socketError),
#endif
    connect(&socket, &QTcpSocket::readyRead,
            this, &JamConnection::socketReadyRead);
}

JamConnection::~JamConnection()
{
}

void JamConnection::connectToServer(const QString &server,
                                    const QString &username,
                                    const QString &hexToken)
{
    if (socket.state() != QAbstractSocket::UnconnectedState) {
        qWarning("%s socket already connected", __func__);
        return;
    }

    QStringList fields = server.split(':');
    if (fields.size() != 2) {
        qWarning("%s expected <server>:<port>, got \"%s\"",
                 __func__, hexToken.toLatin1().constData());
        return;
    }

    bool ok = false;
    quint16 port = fields.at(1).toShort(&ok);
    if (!ok) {
        qWarning("%s invalid port number \"%s\"",
                 __func__, fields.at(1).toLatin1().constData());
        return;
    }

    username_ = username;
    hexToken_ = hexToken;
    error_ = QString{};
    payloadSize = 0;
    socket.connectToHost(fields.at(0), port);
}

void JamConnection::socketConnected()
{
    // Wait for server to send us the auth challenge.  We don't emit connect()
    // until auth is finished.
    qDebug("Socket connected, awaiting auth challenge");
}

void JamConnection::abort()
{
    stopKeepaliveTimers();
    socket.abort();
}

void JamConnection::disconnectFromServer()
{
    stopKeepaliveTimers();

    if (socket.state() != QAbstractSocket::ConnectedState) {
        socket.abort();
    } else {
        socket.disconnectFromHost();
    }
}

void JamConnection::socketDisconnected()
{
    qDebug("Socket disconnected");
    stopKeepaliveTimers();
    emit disconnected();
}

void JamConnection::sendKeepalive()
{
    send(MSG_TYPE_KEEPALIVE, QByteArray{});
}

void JamConnection::keepaliveExpired()
{
    fail(tr("Connection timed out due to server inactivity"));
}

void JamConnection::stopKeepaliveTimers()
{
    sendKeepaliveTimer.stop();
    sendKeepaliveTimer.setInterval(0);
    receiveKeepaliveTimer.stop();
    receiveKeepaliveTimer.setInterval(0);
}

void JamConnection::fail(const QString &errorString)
{
    if (!error_.isNull()) {
        return; // we've already got an error message
    }

    error_ = errorString;
    emit error(error_);

    payloadSize = 0;
    stopKeepaliveTimers();
    socket.abort();
}

void JamConnection::socketError(QAbstractSocket::SocketError)
{
    stopKeepaliveTimers();
    fail(socket.errorString());
}

bool JamConnection::parseMessageHeader()
{
    MessageHeader header;
    if (socket.peek(reinterpret_cast<char*>(&header),
                    sizeof(header)) != sizeof(header)) {
        fail(tr("Unexpected short peek of message header"));
        return false;
    }

    payloadSize = qFromLittleEndian(header.length);
    if (payloadSize > 1 * 1024 * 1024) {
        fail(tr("Payload size %1 is too large").arg(payloadSize));
        return false;
    }
    return true;
}

bool JamConnection::parseAuthChallenge()
{
    struct AuthChallenge
    {
        quint8 challenge[8];
        quint32 serverCapabilities;
        quint32 protocolVersion;
    } Q_PACKED;

    AuthChallenge msg;

    if (payloadSize != sizeof(msg)) {
        fail(tr("Unexpected auth challenge payload size %1").arg(payloadSize));
        return false;
    }

    if (socket.read(reinterpret_cast<char*>(&msg), sizeof(msg)) != sizeof(msg)) {
        fail(tr("Unexpected short read of auth challenge"));
        return false;
    }

    msg.serverCapabilities = qFromLittleEndian(msg.serverCapabilities);
    msg.protocolVersion = qFromLittleEndian(msg.protocolVersion);

    if (msg.protocolVersion != 0x80000000) {
        fail(tr("Unsupported protocol version %1").arg(msg.protocolVersion));
        return false;
    }

    if (msg.serverCapabilities & 0x1) {
        fail(tr("Unsupported license agreement in auth challenge"));
        return false;
    }

    quint8 keepaliveInterval = msg.serverCapabilities >> 8;
    if (keepaliveInterval == 0) {
        keepaliveInterval = 3; // Default value in seconds
    }
    sendKeepaliveTimer.setInterval(keepaliveInterval * 1000);
    receiveKeepaliveTimer.setInterval(3 * keepaliveInterval * 1000);

    return sendAuthUser(msg.protocolVersion, msg.challenge);
}

bool JamConnection::parseAuthReply()
{
    quint8 flag;
    quint8 errorMsgNul;
    quint8 maxChannels_;
    const qint64 minSize = sizeof(flag) + sizeof(errorMsgNul) + sizeof(maxChannels_);

    if (payloadSize < minSize) {
        fail(tr("Auth reply payload size %1 too small").arg(payloadSize));
        return false;
    }

    if (socket.read(reinterpret_cast<char*>(&flag),
                    sizeof(flag)) != sizeof(flag)) {
        fail(tr("Short read of auth reply flag field"));
        return false;
    }

    flag = noEndian8Bit(flag);
    const bool authSuccess = flag & 0x1;

    QString errorMsg;
    const qint64 errorSize = payloadSize - minSize;
    if (errorSize > 0) {
        const QByteArray errorBytes{socket.read(errorSize)};
        if (errorBytes.size() != errorSize) {
            fail(tr("Unexpected auth reply error size %1 (expected %2)").arg(errorBytes.size()).arg(errorSize));
            return false;
        }

        errorMsg = QString::fromUtf8(errorBytes);
    }

    if (socket.read(reinterpret_cast<char *>(&errorMsgNul),
                    sizeof(errorMsgNul)) != sizeof(errorMsgNul)) {
        fail(tr("Short read of auth reply error message NUL byte"));
        return false;
    }
    if (noEndian8Bit(errorMsgNul) != '\0') {
        fail(tr("Expected error message NUL byte, got %1").arg(errorMsgNul));
        return false;
    }

    if (!authSuccess) {
        fail(tr("Authentication failed: %1").arg(errorMsg));
        return false;
    }
    // On success errorMsg may contain an updated username, but we discard it.

    if (socket.read(reinterpret_cast<char*>(&maxChannels_),
                    sizeof(maxChannels_)) != sizeof(maxChannels_)) {
        fail(tr("Short read of auth reply max channels field"));
        return false;
    }
    maxChannels = noEndian8Bit(maxChannels_);

    emit connected();
    return true;
}

bool JamConnection::parseConfigChangeNotify()
{
    struct ConfigChangeNotify
    {
        quint16 bpm;
        quint16 bpi;
    } Q_PACKED;

    if (payloadSize != sizeof(ConfigChangeNotify)) {
        fail(tr("Invalid config change notify payload size %1").arg(payloadSize));
        return false;
    }

    ConfigChangeNotify msg;
    if (socket.read(reinterpret_cast<char*>(&msg),
                    sizeof(msg)) != sizeof(msg)) {
        fail(tr("Short read of config change notify"));
        return false;
    }

    emit configChanged(qFromLittleEndian(msg.bpm),
                       qFromLittleEndian(msg.bpi));
    return true;
}

bool JamConnection::parseUserInfoChangeNotify()
{
    struct UserInfoChangeNotify
    {
        quint8 active;
        quint8 channelIndex;
        qint16 volume;
        qint8 pan;
        quint8 flags;
    } Q_PACKED;

    QList<UserInfo> list;
    QByteArray bytes = socket.read(payloadSize);
    if (bytes.size() != payloadSize) {
        fail(tr("Short read of user info change notify"));
        return false;
    }

    // Looping over a variable length list of variable length structures.
    // Let's go old-school for this.
    const char *p = bytes.constData();
    const char *end = bytes.constData() + bytes.size();
    while (p < end) {
        if (p + sizeof(UserInfoChangeNotify) > end) {
            fail(tr("Short user info change notify structure"));
            return false;
        }

        UserInfoChangeNotify item;
        memcpy(&item, p, sizeof(item));
        item.active = noEndian8Bit(item.active);
        item.channelIndex = noEndian8Bit(item.channelIndex);
        item.volume = qFromLittleEndian(item.volume);
        item.pan = noEndian8Bit(item.pan);
        item.flags = noEndian8Bit(item.flags);
        p += sizeof(item);

        const char *usernamePtr = p;
        while (p < end && *p != '\0') {
            p++;
        }
        if (p == end) {
            fail(tr("Short user info change notify username"));
            return false;
        }
        p++; // step over the NUL-terminator

        const char *channelNamePtr = p;
        while (p < end && *p != '\0') {
            p++;
        }
        if (p == end) {
            fail(tr("Short user info change notify channel name"));
            return false;
        }
        p++; // step over the NUL-terminator

        UserInfo userInfo;
        userInfo.active = item.active;
        userInfo.channelIndex = item.channelIndex;
        userInfo.volume = item.volume;
        userInfo.pan = item.pan;
        userInfo.pan = item.flags;
        userInfo.username = QString::fromUtf8(usernamePtr);
        userInfo.channelName = QString::fromUtf8(channelNamePtr);

        if (userInfo.channelIndex >= maxChannels) {
            fail(tr("Channel index %1 invalid with max channels %2 for user \"%3\"").arg(userInfo.channelIndex).arg(maxChannels).arg(userInfo.username));
            return false;
        }

        list.append(userInfo);
    }

    emit userInfoChanged(list);
    return true;
}

bool JamConnection::parseDownloadIntervalBegin()
{
    struct DownloadIntervalBegin
    {
        Guid guid;
        quint32 estimatedSize;
        FourCC fourCC;
        quint8 channelIndex;
    } Q_PACKED;
    const qint64 minSize = sizeof(DownloadIntervalBegin);

    if (payloadSize < minSize) {
        fail(tr("Payload size for download interval begin too small %1").arg(payloadSize));
        return false;
    }

    QByteArray bytes = socket.read(payloadSize);
    if (bytes.size() != payloadSize) {
        fail(tr("Short read in download interval begin"));
        return false;
    }

    DownloadIntervalBegin msg;
    memcpy(&msg, bytes.constData(), sizeof(msg));
    msg.estimatedSize = qFromLittleEndian(msg.estimatedSize);
    msg.channelIndex = noEndian8Bit(msg.channelIndex);
    bytes.remove(0, sizeof(msg));

    if (bytes.size() < 1) {
        fail(tr("Missing username field in download interval begin"));
        return false;
    }
    if (!bytes.endsWith('\0')) {
        fail(tr("Expected username NUL terminator in download interval begin, got %1").arg(bytes.back()));
        return false;
    }
    bytes.remove(bytes.size() - 1, 1);

    const QString username = QString::fromUtf8(bytes);

    if (msg.channelIndex >= maxChannels) {
        fail(tr("Download interval begin channel index %1 invalid with max channels %2 for user \"%3\"").arg(msg.channelIndex).arg(maxChannels).arg(username));
        return false;
    }

    emit downloadIntervalBegan(msg.guid, msg.estimatedSize, msg.fourCC,
                               msg.channelIndex, username);
    return true;
}

bool JamConnection::parseDownloadIntervalWrite()
{
    const qint64 fieldSize = sizeof(Guid) + sizeof(quint8);

    if (payloadSize < fieldSize) {
        fail(tr("Payload size for download interval write too small %1").arg(payloadSize));
        return false;
    }

    QByteArray bytes = socket.read(payloadSize);
    if (bytes.size() != payloadSize) {
        fail(tr("Short read in download interval write"));
        return false;
    }

    quint8 flags = noEndian8Bit(bytes.at(sizeof(Guid)));
    QByteArray audioData;
    if (!(flags & 0x1)) {
        audioData.setRawData(bytes.constData() + fieldSize,
                             payloadSize - fieldSize);
    }

    emit downloadIntervalReceived(reinterpret_cast<const quint8*>(bytes.constData()),
                                  audioData);
    return true;
}

bool JamConnection::parseChatMessage()
{
    QByteArray payload = socket.read(payloadSize);
    if (payload.size() != payloadSize) {
        fail(tr("Short read in chat message"));
        return false;
    }

    QList<QByteArray> fields = payload.split('\0');
    if (fields.size() != 6) {
        fail(tr("Invalid chat message fields received"));
        return false;
    }

    emit chatMessageReceived(QString::fromUtf8(fields.at(0)),
                             QString::fromUtf8(fields.at(1)),
                             QString::fromUtf8(fields.at(2)),
                             QString::fromUtf8(fields.at(3)),
                             QString::fromUtf8(fields.at(4)));
    return true;
}

bool JamConnection::parseKeepalive()
{
    if (payloadSize != 0) {
        fail(tr("Expected empty keepalive payload"));
        return false;
    }
    return true;
}

bool JamConnection::parseMessage()
{
    MessageHeader header;
    if (socket.read(reinterpret_cast<char*>(&header), sizeof(header)) != sizeof(header)) {
        fail(tr("Unexpected short read of message header"));
        return false;
    }

    switch (noEndian8Bit(header.type)) {
    case MSG_TYPE_SERVER_AUTH_CHALLENGE:
        return parseAuthChallenge();
    case MSG_TYPE_SERVER_AUTH_REPLY:
        return parseAuthReply();
    case MSG_TYPE_SERVER_CONFIG_CHANGE_NOTIFY:
        return parseConfigChangeNotify();
    case MSG_TYPE_SERVER_USERINFO_CHANGE_NOTIFY:
        return parseUserInfoChangeNotify();
    case MSG_TYPE_SERVER_DOWNLOAD_INTERVAL_BEGIN:
        return parseDownloadIntervalBegin();
    case MSG_TYPE_SERVER_DOWNLOAD_INTERVAL_WRITE:
        return parseDownloadIntervalWrite();
    case MSG_TYPE_CHAT_MESSAGE:
        return parseChatMessage();
    case MSG_TYPE_KEEPALIVE:
        return parseKeepalive();
    default:
        fail(tr("Invalid message type %#x").arg(noEndian8Bit(header.type)));
        return false;
    }
}

void JamConnection::socketReadyRead()
{
    if (receiveKeepaliveTimer.interval() > 0) {
        receiveKeepaliveTimer.start();
    }

    // Consume all messages available right now but don't fetch
    // bytesAvailable() again (the next readyRead() signal will do that)
    qint64 available = socket.bytesAvailable();
    const qint64 messageHeaderSize = sizeof(MessageHeader);

    while (available >= messageHeaderSize + payloadSize) {
        if (payloadSize == 0) {
            if (!parseMessageHeader()) {
                return;
            }
        }

        if (available < messageHeaderSize + payloadSize) {
            return;
        }

        bool ok = parseMessage();
        if (!ok) {
            return;
        }

        available -= messageHeaderSize + payloadSize;
        payloadSize = 0;
    }
}

bool JamConnection::send(quint8 type, const char *data, size_t len)
{
    const MessageHeader header = {
        .type = noEndian8Bit(type),
        .length = qToLittleEndian(static_cast<quint32>(len)),
    };

    if (socket.write(reinterpret_cast<const char*>(&header),
                     sizeof(header)) != sizeof(header)) {
        fail(tr("Short write of header to socket"));
        return false;
    }

    const qint64 expectedLen = len;
    if (socket.write(data, len) != expectedLen) {
        fail(tr("Short write of %1 bytes to socket").arg(len));
        return false;
    }

    if (sendKeepaliveTimer.interval() > 0) {
        sendKeepaliveTimer.start();
    }
    return true;
}

bool JamConnection::send(quint8 type, const QByteArray &bytes)
{
    return send(type, bytes.constData(), bytes.size());
}

bool JamConnection::sendAuthUser(quint32 protocolVersion, const quint8 challenge[8])
{
    protocolVersion = qToLittleEndian(protocolVersion);

    const QByteArray username = username_.toUtf8();
    const quint32 clientCapabilities = qToLittleEndian(0);

    // Password hash = SHA1(SHA1(username + ":" + hexToken) + challenge)
    QCryptographicHash inner{QCryptographicHash::Sha1};
    QCryptographicHash outer{QCryptographicHash::Sha1};
    inner.addData(username);
    inner.addData(":", 1);
    inner.addData(hexToken_.toUtf8());
    outer.addData(inner.result());
    outer.addData(reinterpret_cast<const char*>(challenge), 8);
    const QByteArray passwordHash{outer.result()};

    // Build message
    QByteArray bytes;
    bytes.append(passwordHash);
    bytes.append(username);
    bytes.append('\0');
    bytes.append(reinterpret_cast<const char*>(&clientCapabilities),
                 sizeof(clientCapabilities));
    bytes.append(reinterpret_cast<const char*>(&protocolVersion),
                 sizeof(protocolVersion));

    return send(MSG_TYPE_CLIENT_AUTH_USER, bytes);
}

bool JamConnection::sendSetUsermask(QString const &username,
                                    const quint32* mask,
                                    size_t maskSize)
{
    const QByteArray usernameBytes{username.toUtf8()};

    QByteArray bytes;
    bytes.append(usernameBytes);
    bytes.append('\0');

    for (size_t i = 0; i < maskSize; i++) {
        bytes.append(qToLittleEndian(mask[i]));
    }

    return send(MSG_TYPE_CLIENT_SET_USERMASK, bytes);
}

bool JamConnection::sendChannelInfo(QList<JamConnection::ChannelInfo> const &channels)
{
    QByteArray bytes;

    const quint16 paramSize = qToLittleEndian(
            sizeof(qint16) /* volume */ +
            sizeof(qint8) /* pan */ +
            sizeof(quint8) /* flags */);
    bytes.append(reinterpret_cast<const char*>(&paramSize), sizeof(paramSize));

    for (int i = 0; i < channels.size(); i++) {
        const ChannelInfo &channel = channels.at(i);
        const qint16 volume = qToLittleEndian(channel.volume);

        bytes.append(channel.name.toUtf8());
        bytes.append('\0');
        bytes.append(reinterpret_cast<const char*>(&volume), sizeof(volume));
        bytes.append(noEndian8Bit(channel.pan));
        bytes.append(noEndian8Bit(channel.flags));
    }

    return send(MSG_TYPE_CLIENT_SET_CHANNEL_INFO, bytes);
}

bool JamConnection::sendUploadIntervalBegin(const Guid guid,
                                            quint32 estimatedSize,
                                            const FourCC fourCC,
                                            quint8 channelIndex)
{
    struct UploadIntervalBegin
    {
        Guid guid;
        quint32 estimatedSize;
        FourCC fourCC;
        quint8 channelIndex;
    } Q_PACKED;

    UploadIntervalBegin msg;
    memcpy(msg.guid, guid, sizeof(Guid));
    msg.estimatedSize = qToLittleEndian(estimatedSize);
    memcpy(msg.fourCC, fourCC, sizeof(FourCC));
    msg.channelIndex = noEndian8Bit(channelIndex);

    return send(MSG_TYPE_CLIENT_UPLOAD_INTERVAL_BEGIN,
                reinterpret_cast<const char*>(&msg),
                sizeof(msg));
}

bool JamConnection::sendUploadIntervalWrite(const Guid guid,
                                            quint8 flags,
                                            const quint8 *data,
                                            qint64 len)
{
    struct UploadIntervalWrite
    {
        Guid guid;
        uint8_t flags;
    } Q_PACKED;

    UploadIntervalWrite msg;
    memcpy(msg.guid, guid, sizeof(Guid));
    msg.flags = noEndian8Bit(flags);

    if (!send(MSG_TYPE_CLIENT_UPLOAD_INTERVAL_WRITE,
              reinterpret_cast<const char*>(&msg),
              sizeof(msg))) {
        return false;
    }

    // Plain socket.write() here to avoid copying audio data
    if (socket.write(reinterpret_cast<const char*>(data), len) != len) {
        fail(tr("Short write of %1 bytes of audio data").arg(len));
        return false;
    }
    return true;
}

bool JamConnection::sendChatMessage(const QString &command,
                                    const QString &arg1,
                                    const QString &arg2,
                                    const QString &arg3,
                                    const QString &arg4)
{
    QByteArray bytes;
    bytes.append(command.toUtf8());
    bytes.append('\0');
    bytes.append(arg1.toUtf8());
    bytes.append('\0');
    bytes.append(arg2.toUtf8());
    bytes.append('\0');
    bytes.append(arg3.toUtf8());
    bytes.append('\0');
    bytes.append(arg4.toUtf8());
    bytes.append('\0');
    return send(MSG_TYPE_CHAT_MESSAGE, bytes);
}
