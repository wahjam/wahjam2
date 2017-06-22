#include <QUuid>
#include <QCryptographicHash>
#include "JamApiManager.h"

JamApiManager::JamApiManager(QObject *parent)
    : QObject{parent},
      netManager{new QNetworkAccessManager{this}},
      apiUrl{"https://jammr.net/api/"}, // TODO make this a setting
      loggedIn{false}
{
}

void JamApiManager::login()
{
    QUrl url{QString("tokens/%1/").arg(username_)};

    /* Generate unique 20-byte token */
    hexToken_ = QCryptographicHash::hash(QUuid::createUuid().toRfc4122(),
                                         QCryptographicHash::Sha1).toHex();

    loginReply = post(url, QString("token=%1").arg(hexToken_).toLatin1());
    connect(loginReply, SIGNAL(finished()),
            this, SLOT(loginRequestFinished()));
}

void JamApiManager::loginRequestFinished()
{
    QNetworkReply *reply = loginReply;
    loginReply->deleteLater();
    loginReply = nullptr;

    loginError_ = QString{};
    switch (reply->error()) {
    case QNetworkReply::NoError:
    {
        QVariant redirect = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
        if (redirect.isValid()) {
            loginError_ = tr("Unexpected redirect to \"%1\".  Please report this error.").arg(redirect.toUrl().toString());
        }
        break;
    }
    case QNetworkReply::AuthenticationRequiredError:
        if (username_.contains('@')) {
            loginError_ = tr("Invalid username/password.  Please check that you are using the username you registered with and not your email address.");
        } else {
            loginError_ = tr("Invalid username/password, please try again.");
        }
        break;
    default:
        loginError_ = tr("Unable to connect.  Please check your internet connection and try again.  If this problem persists, please report error number %1").arg(reply->error());
        break;
    }

    loggedIn = loginError_.isNull();
    emit loginFinished();
}

bool JamApiManager::isLoggedIn() const
{
    return loggedIn;
}

QString JamApiManager::username() const
{
    return username_;
}

void JamApiManager::setUsername(const QString &username)
{
    loggedIn = false;
    username_ = username;
    emit usernameChanged();
}

QString JamApiManager::password() const
{
    return password_;
}

void JamApiManager::setPassword(const QString &password)
{
    loggedIn = false;
    password_ = password;
    emit passwordChanged();
}

QString JamApiManager::hexToken() const
{
    return hexToken_;
}

QString JamApiManager::loginError() const
{
    return loginError_;
}

QNetworkReply *JamApiManager::get(const QUrl &relativeUrl)
{
    if (!relativeUrl.isRelative()) {
        qDebug("%s expected relative URL", __func__);
        return nullptr;
    }

    QUrl url{apiUrl.resolved(relativeUrl)};
    url.setUserName(username_);
    url.setPassword(password_);

    QNetworkRequest request{url};
    request.setRawHeader("Referer",
                         url.toString(QUrl::RemoveUserInfo).toLatin1().data());
    return netManager->get(request);
}

QNetworkReply *JamApiManager::post(const QUrl &relativeUrl, const QByteArray &data)
{
    if (!relativeUrl.isRelative()) {
        qDebug("%s expected relative URL", __func__);
        return nullptr;
    }

    QUrl url{apiUrl.resolved(relativeUrl)};
    url.setUserName(username_);
    url.setPassword(password_);

    QNetworkRequest request{url};
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      "application/x-www-form-urlencoded");
    request.setRawHeader("Referer",
                         url.toString(QUrl::RemoveUserInfo).toLatin1().data());
    return netManager->post(request, data);
}
