// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <QAbstractListModel>
#include "JamApiManager.h"

class SessionListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(JamApiManager *jamApiManager READ jamApiManager WRITE setJamApiManager NOTIFY jamApiManagerChanged)

public:
    SessionListModel(QObject *parent = nullptr);
    ~SessionListModel();

    int rowCount(const QModelIndex &parent) const Q_DECL_OVERRIDE;
    QVariant data(const QModelIndex &index, int role) const Q_DECL_OVERRIDE;
    QHash<int, QByteArray> roleNames() const Q_DECL_OVERRIDE;

    JamApiManager *jamApiManager() const;
    void setJamApiManager(JamApiManager *jamApiManager);

signals:
    void dataReady();
    void jamApiManagerChanged();

public slots:
    void refresh();

private:
    struct SessionItem {
        QString server;     // "hostname:port"
        QString topic;      // "Public jam - play nicely"
        QString tempo;      // "120 BPM/16 BPI"
        bool isPublic;      // true - public jam, false - private jam
        QString slots_;     // "1/8"
        QString users;      // "alex, bob, chris"
    };

    QList<SessionItem> items;
    JamApiManager *jamApiManager_;
    QNetworkReply *reply;

    void updateItems(const QList<SessionItem> &newItems);

private slots:
    void replyFinished();
};
