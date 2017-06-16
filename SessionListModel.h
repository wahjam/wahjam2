#pragma once

#include <QNetworkAccessManager>
#include <QAbstractListModel>

class SessionListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    SessionListModel(QObject *parent = nullptr);
    ~SessionListModel();

    int rowCount(const QModelIndex &parent) const Q_DECL_OVERRIDE;
    QVariant data(const QModelIndex &index, int role) const Q_DECL_OVERRIDE;
    QHash<int, QByteArray> roleNames() const Q_DECL_OVERRIDE;

signals:
    void dataReady();

private:
    struct SessionItem {
        QString server;     // "hostname:port"
        QString topic;      // "Public jam - play nicely"
        QString tempo;      // "120 BPM/16 BPI"
        QString slots_;     // "1/8"
        QString users;      // "alex, bob, chris"
    };

    QList<SessionItem> items;
    QNetworkReply *reply;

    void sendNetworkRequest(QNetworkAccessManager *netManager, const QUrl &apiUrl);
    void updateItems(const QList<SessionItem> &newItems);

private slots:
    void replyFinished();
};
