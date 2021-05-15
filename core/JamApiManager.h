// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <QUrl>
#include <QNetworkReply>
#include <QNetworkAccessManager>

/*
 * This class is a HTTP request factory for jam REST API calls.  The user must
 * be logged in before sending API requests.
 *
 * To log in, set the username and password properties and invoke login().
 * When the loginFinished() signal is emitted check that isLoggedIn() is true.
 * If login failed the human-readable error message can be fetched from the
 * loginError property.
 */
class JamApiManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString username READ username WRITE setUsername NOTIFY usernameChanged)
    Q_PROPERTY(QString password READ password WRITE setPassword NOTIFY passwordChanged)
    Q_PROPERTY(bool rememberPassword READ rememberPassword WRITE setRememberPassword NOTIFY rememberPasswordChanged)
    Q_PROPERTY(QString hexToken READ hexToken NOTIFY hexTokenChanged)
    Q_PROPERTY(QString loginError READ loginError NOTIFY loginFinished)

public:
    JamApiManager(QObject *parent = nullptr);

    /*
     * Log in to REST API using given username and password.  The
     * loginFinished() signal is emitted on success.  The loginError() signal
     * is emitted on failure.
     */
    Q_INVOKABLE void login();
    Q_INVOKABLE bool isLoggedIn() const;

    // Create a new private jam session and emit privateJamCreated() with the
    // server address on success or privateJamCreationFailed() on failure.
    Q_INVOKABLE void createPrivateJam();

    QString username() const;
    void setUsername(const QString &username);
    QString password() const;
    void setPassword(const QString &password);
    bool rememberPassword() const;
    void setRememberPassword(bool enable);
    QString hexToken() const;
    QString loginError() const;

    // HTTP requests
    QNetworkReply *get(const QUrl &relativeUrl);
    QNetworkReply *post(const QUrl &relativeUrl, const QByteArray &data);

signals:
    void usernameChanged();
    void passwordChanged();
    void rememberPasswordChanged();
    void hexTokenChanged();
    void loginFinished();
    void createPrivateJamFinished(const QString &server);
    void createPrivateJamFailed(const QString &errmsg);

private:
    QNetworkAccessManager *netManager;
    QUrl apiUrl;
    QString username_;
    QString password_;
    bool rememberPassword_;
    QString hexToken_;
    QString loginError_;
    QNetworkReply *loginReply;
    bool loggedIn;

private slots:
    void loginRequestFinished();
};
