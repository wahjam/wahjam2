#include "SessionListModel.h"

enum {
    ServerRole  = Qt::UserRole,
    TopicRole   = Qt::UserRole + 1,
    TempoRole   = Qt::UserRole + 2,
    SlotsRole   = Qt::UserRole + 3,
    UsersRole   = Qt::UserRole + 4,
};

SessionListModel::SessionListModel(QObject *parent)
    : QAbstractListModel{parent}
{
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
