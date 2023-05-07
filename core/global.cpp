// SPDX-License-Identifier: Apache-2.0
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QLoggingCategory>
#include <QStandardPaths>
#include <QSysInfo>

#include "config.h"
#include "global.h"

static FILE *logfp = stderr;

QString logFilePath;

static void qtMessageHandler(QtMsgType type,
                             const QMessageLogContext &context,
                             const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();

    Q_UNUSED(context);

    const char *msgtype;
    switch (type) {
    case QtDebugMsg:
        msgtype = "DEBUG";
        break;
    case QtWarningMsg:
        msgtype = "WARN";
        break;
    case QtCriticalMsg:
        msgtype = "CRIT";
        break;
    case QtFatalMsg:
        msgtype = "FATAL";
        break;
    default:
        msgtype = "???";
        break;
    }

    QString timestamp(QDateTime::currentDateTime().toUTC().toString("MMM dd yyyy hh:mm:ss "));

    fprintf(logfp, "%s %s %s\n",
            timestamp.toUtf8().constData(),
            msgtype,
            localMsg.constData());
    if (type == QtFatalMsg) {
        abort();
    }
}

void installMessageHandler()
{
    QLoggingCategory::setFilterRules("*.debug=true\nqt.*.debug=false");
    qInstallMessageHandler(qtMessageHandler);
}

void globalInit()
{
    QCoreApplication::setOrganizationName(ORGNAME);
    QCoreApplication::setOrganizationDomain(ORGDOMAIN);
    QCoreApplication::setApplicationName(APPNAME);
    QCoreApplication::setApplicationVersion(VERSION);

    const QDir dataDir{QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)};
    if (!dataDir.mkpath(dataDir.absolutePath())) {
        fprintf(stderr, "Unable to create application data directory at %s.\n",
                dataDir.absolutePath().toLocal8Bit().constData());
        abort();
    }

    logFilePath = dataDir.filePath("log.txt");
    logfp = fopen(logFilePath.toLocal8Bit().constData(), "w");
    if (!logfp) {
        logfp = stderr;
    }

    // Disable buffering so messages are saved even if there is a crash
    setvbuf(logfp, nullptr, _IONBF, 0);

    qDebug("%s %s", APPNAME, VERSION);
    qDebug("Qt %s", qVersion());
    qDebug("OS: %s", QSysInfo::prettyProductName().toLatin1().constData());
    qDebug("CPU: %s", QSysInfo::currentCpuArchitecture().toLatin1().constData());
}

void globalCleanup()
{
    if (logfp != stderr) {
        fclose(logfp);
        logfp = stderr;
    }
}
