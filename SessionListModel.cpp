#include <QNetworkReply>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

#include "SessionListModel.h"

enum {
    ServerRole  = Qt::UserRole,
    TopicRole   = Qt::UserRole + 1,
    TempoRole   = Qt::UserRole + 2,
    SlotsRole   = Qt::UserRole + 3,
    UsersRole   = Qt::UserRole + 4,
};

SessionListModel::SessionListModel(QObject *parent)
    : QAbstractListModel{parent}, reply{nullptr}
{
    sendNetworkRequest(new QNetworkAccessManager{this}, QUrl{"https://jammr.net/api/"});
}

SessionListModel::~SessionListModel()
{
    if (reply) {
        reply->abort();
    }
}

int SessionListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return items.count();
}

QVariant SessionListModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    if (row < 0 || row >= items.count()) {
        return QVariant{};
    }

    const SessionItem &item = items.at(row);
    switch (role) {
    case ServerRole:
        return item.server;
    case TopicRole:
        return item.topic;
    case TempoRole:
        return item.tempo;
    case SlotsRole:
        return item.slots_;
    case UsersRole:
        return item.users;
    default:
        return QVariant{};
    }
}

QHash<int, QByteArray> SessionListModel::roleNames() const
{
    QHash<int, QByteArray> names;
    names[ServerRole] = "server";
    names[TopicRole] = "topic";
    names[TempoRole] = "tempo";
    names[SlotsRole] = "slots";
    names[UsersRole] = "users";
    return names;
}

void SessionListModel::updateItems(const QList<SessionItem> &newItems)
{
    /*
     * It would be possible to insert/remove individual rows depending on the
     * difference between the new items and old items.  That would allow views
     * to update gracefully but is more complex to implement.  Just discard the
     * old items and switch to the new items instead.
     */
    beginResetModel();
    items = newItems;
    endResetModel();
}

void SessionListModel::sendNetworkRequest(QNetworkAccessManager *netManager, const QUrl &apiUrl)
{
    if (reply) {
        return;
    }

    QUrl livejamsUrl{apiUrl};
    livejamsUrl.setPath(apiUrl.path() + "livejams/");

    QNetworkRequest request{livejamsUrl};
    request.setRawHeader("Referer", livejamsUrl.toString(QUrl::RemoveUserInfo).toLatin1().data());

    reply = netManager->get(request);
    connect(reply, SIGNAL(finished()),
            this, SLOT(replyFinished()));
    connect(reply, SIGNAL(finished()),
            reply, SLOT(deleteLater()));
}

void SessionListModel::replyFinished()
{
    QJsonParseError err;
    QTextStream stream{reply};
    QJsonDocument doc{QJsonDocument::fromJson(stream.device()->readAll(), &err)};

    reply = nullptr;

    if (doc.isNull()) {
        qCritical("Server list JSON parse error: %s",
                  err.errorString().toLatin1().constData());
        return;
    }

    qDebug("%s succeeded", __func__);

    /* The JSON looks like this:
     *
     * [
     *   {
     *     'server': 'host:port',
     *     'users': ['bob', 'joe'],
     *     'topic': 'Hello world!',
     *     'bpm': 120,
     *     'bpi': 16,
     *     'is_public': true,
     *     'numusers': 2,
     *     'maxusers': 6
     *   },
     *   ...
     * ]
     */

    QList<SessionItem> newItems;

    foreach (QJsonValue jam, doc.array()) {
        QJsonObject obj{jam.toObject()};
        QString server = obj.value("server").toString();
        if (server.isEmpty()) {
            continue; // skip invalid element
        }

        QString topic = obj.value("topic").toString();
        double bpm = obj.value("bpm").toDouble();
        double bpi = obj.value("bpi").toDouble();
        double numUsers = obj.value("numusers").toDouble();
        double maxUsers = obj.value("maxusers").toDouble();

        QStringList users;
        foreach (QJsonValue user, obj.value("users").toArray()) {
            users << user.toString();
        }

        newItems.append({
            .server = server,
            .topic = topic,
            .tempo = QString{"%1 BPM/%2 BPI"}.arg(bpm).arg(bpi),
            .slots_ = QString{"%1/%2"}.arg(numUsers).arg(maxUsers),
            .users = users.join(", "),
        });
    }

    updateItems(newItems);
}
