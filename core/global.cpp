// SPDX-License-Identifier: Apache-2.0
#include <QCoreApplication>
#include <QDir>
#include <QQmlEngine>
#include <QStandardPaths>

#include "global.h"
#include "SessionListModel.h"
#include "JamApiManager.h"

static FILE *logfp = stderr;

static void qtMessageHandler(QtMsgType type,
                             const QMessageLogContext &context,
                             const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();

    Q_UNUSED(context);

    fprintf(logfp, "%s\n", localMsg.constData());
    if (type == QtFatalMsg) {
        abort();
    }
}

void installMessageHandler()
{
    qInstallMessageHandler(qtMessageHandler);
}

void globalInit()
{
    QCoreApplication::setOrganizationName("Wahjam Project");
    QCoreApplication::setOrganizationDomain("wahjam.org");
    QCoreApplication::setApplicationName("Wahjam2");

    const QDir documents{QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)};
    const QString log_txt{documents.filePath("log.txt")};
    logfp = fopen(log_txt.toLocal8Bit().constData(), "w");
    if (!logfp) {
        logfp = stderr;
    }

    // Disable buffering so messages are saved even if there is a crash
    setvbuf(logfp, nullptr, _IONBF, 0);
}

void globalCleanup()
{
    if (logfp != stderr) {
        fclose(logfp);
        logfp = stderr;
    }
}

// TODO how to unregister types?
void registerQmlTypes()
{
    qmlRegisterType<SessionListModel>("org.wahjam.client", 1, 0, "SessionListModel");
    qmlRegisterType<JamApiManager>("org.wahjam.client", 1, 0, "JamApiManager");
}
