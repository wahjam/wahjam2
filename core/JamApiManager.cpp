// SPDX-License-Identifier: Apache-2.0
#include <QCryptographicHash>
#include <QEventLoop>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSettings>
#include <QUuid>
#ifdef HAVE_QT5KEYCHAIN_H
#include <qt5keychain/keychain.h>
#endif
#include "config.h"
#include "JamApiManager.h"

#if HAVE_QT5KEYCHAIN_H
static QString keychainGetPassword()
{
  QKeychain::ReadPasswordJob job(ORGDOMAIN);
  QEventLoop loop;

  job.setAutoDelete(false);
  job.connect(&job, SIGNAL(finished(QKeychain::Job*)), &loop, SLOT(quit()));
  job.setKey("password");
  job.start();
  loop.exec();

  return job.textData();
}

static bool keychainSetPassword(const QString &password)
{
  QKeychain::WritePasswordJob job(ORGDOMAIN);
  QEventLoop loop;

  job.setAutoDelete(false);
  job.connect(&job, SIGNAL(finished(QKeychain::Job*)), &loop, SLOT(quit()));
  job.setKey("password");
  job.setTextData(password);
  job.start();
  loop.exec();
  return true;
}

static bool keychainDeletePassword()
{
  QKeychain::DeletePasswordJob job(ORGDOMAIN);
  QEventLoop loop;

  job.setAutoDelete(false);
  job.connect(&job, SIGNAL(finished(QKeychain::Job*)), &loop, SLOT(quit()));
  job.setKey("password");
  job.start();
  loop.exec();
  return true;
}
#else
static QString keychainGetPassword()
{
    return {};
}

static bool keychainSetPassword(const QString &password)
{
    return false;
}

static bool keychainDeletePassword()
{
    return false;
}
#endif // HAVE_QT5KEYCHAIN_H

JamApiManager::JamApiManager(QObject *parent)
    : QObject{parent},
      netManager{new QNetworkAccessManager{this}},
      loginReply{nullptr},
      createPrivateJamReply{nullptr},
      loggedIn{false}
{
    QSettings settings;

    settings.beginGroup("login");

    apiUrl = settings.value("apiUrl", "https://jammr.net/api/").toString();
    upgradeUrl = settings.value("upgradeUrl", "https://jammr.net/payments/subscribe").toString();
    username_ = settings.value("username").toString();

    if (settings.value("passwordSaved", false).toBool()) {
        password_ = keychainGetPassword();
    }

    rememberPassword_ = settings.value("rememberPassword", true).toBool();
}

void JamApiManager::login()
{
    if (loginReply) {
        qWarning("Only one login operation can run at a time");
        return;
    }

    QSettings settings;

    settings.beginGroup("login");
    settings.setValue("username", username_);

    if (rememberPassword_) {
        if (keychainSetPassword(password_)) {
            settings.setValue("passwordSaved", true);
        }
    } else if (settings.value("passwordSaved", false).toBool()) {
        /* If it was previously saved, delete it now */
        if (keychainDeletePassword()) {
            settings.setValue("passwordSaved", false);
        }
    }

    settings.setValue("rememberPassword", rememberPassword_);

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
        loginError_ = tr("Unable to connect.  Please check your internet connection and try again.  If this problem persists, please report error number %1.").arg(reply->error());
        break;
    }

    loggedIn = loginError_.isNull();
    if (loggedIn) {
        qDebug("%s success", __func__);
    } else {
        qDebug("%s error \"%s\"", __func__, loginError_.toLatin1().constData());
    }

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

bool JamApiManager::rememberPassword() const
{
    return rememberPassword_;
}

void JamApiManager::setRememberPassword(bool enable)
{
    rememberPassword_ = enable;
    emit rememberPasswordChanged();
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

    if (data.isEmpty()) {
        // NetworkAccessManager::post() requires non-empty data
        return netManager->sendCustomRequest(request, "POST");
    } else {
        return netManager->post(request, data);
    }
}

void JamApiManager::createPrivateJam()
{
    if (createPrivateJamReply) {
        qWarning("Only one private jam can be created at a time");
        return;
    }

    qDebug("Sending request to create private jam...");

    createPrivateJamReply = post(QUrl("livejams/"), QByteArray{});
    connect(createPrivateJamReply, &QNetworkReply::finished,
            this, &JamApiManager::createPrivateJamRequestFinished);
}

void JamApiManager::createPrivateJamRequestFinished()
{
    auto reply = createPrivateJamReply;
    createPrivateJamReply->deleteLater();
    createPrivateJamReply = nullptr;

    auto netErr = reply->error();
    if (netErr != QNetworkReply::NoError) {
        qCritical("Create private jam network reply failed with error %d",
                  netErr);

        QString errmsg = netErr == QNetworkReply::AuthenticationRequiredError ?
            tr("Upgrade to premium <a href=\"%1\">here</a> to create private jams.").arg(upgradeUrl.toString()) :
            tr("Network error %1. Please ensure you are connected to the internet and try again.").arg(netErr);

        emit createPrivateJamFailed(errmsg);
        return;
    }

    QJsonParseError err;
    QTextStream stream{reply};
    QJsonDocument doc{QJsonDocument::fromJson(stream.device()->readAll(), &err)};

    if (doc.isNull()) {
        qCritical("Failed to create private jam: %s",
                  err.errorString().toLatin1().constData());
        emit createPrivateJamFailed(err.errorString());
        return;
    }

    /* The JSON looks like this:
     *
     * {
     *   "server": "host:port"
     * }
     */

    QString server = doc.object().value("server").toString();
    if (server.isNull()) {
        QString errmsg = tr("Unable to create private jam due to missing \"server\" field in JSON. Please try again and report this bug if it continues to happen.");
        qCritical("Failed to parse \"server\" field from JSON: %s",
                  errmsg.toLatin1().constData());
        emit createPrivateJamFailed(errmsg);
        return;
    }

    qDebug("Successfully created private jam at %s",
           server.toLatin1().constData());
    emit createPrivateJamFinished(server);
}
