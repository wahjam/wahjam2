// SPDX-License-Identifier: Apache-2.0
#include <thread>
#include <future>
#include <QGuiApplication>
#include <QQmlEngine>
#include <QThread>
#include <QDir>
#include <QStandardPaths>

#include "global.h"
#include "SessionListModel.h"
#include "JamApiManager.h"

// How many instances have initialized us?
static unsigned int initCount;

/*
 * VST plugins have no periodic callback suitable for Qt event loop
 * integration. effEditIdle is only called while the editor GUI is open and
 * therefore cannot be used for the Qt event loop.
 *
 * If qGuiApp has already been initialized then we can use the existing Qt event loop.
 *
 * If it hasn't been initialized then its our responsibility to create the Qt
 * event loop. Spawn a new thread to run the event loop.
 */
static std::thread *qtThread;

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

static void qtThreadRun(std::promise<void> ready)
{
    /*
     * Qt relies on this data remaining accessible across subsequent
     * QCoreApplication singleton instances so make it static.
     */
    static char progname[] = {'V', 'S', 'T', '\0'};
    static char *argv = progname;
    static int argc = 1;

    qInstallMessageHandler(qtMessageHandler);

    QGuiApplication app{argc, &argv};

    QCoreApplication::setOrganizationName("The Wahjam Project");
    QCoreApplication::setOrganizationDomain("wahjam.org");
    QCoreApplication::setApplicationName("Wahjam2");

    const QDir documents{QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)};
    const QString log_txt{documents.filePath("log.txt")};
    logfp = fopen(log_txt.toLocal8Bit().constData(), "w");
    if (!logfp) {
        logfp = stderr;
    }

    app.setQuitOnLastWindowClosed(false);
    QObject::connect(&app, &QGuiApplication::aboutToQuit,
                     []() { qDebug("qtThreadRun done"); });

    qDebug("%s threadId %p", __func__, QThread::currentThreadId());

    ready.set_value();

    app.exec();
}

static void qtThreadJoin()
{
    qtThread->join();
    qDebug("%s joined", __func__);
    delete qtThread;
    qDebug("%s deleted Qt thread", __func__);
    qtThread = nullptr;
}

static void qtThreadStart()
{
    if (qGuiApp || qtThread) {
        return;
    }

    std::promise<void> ready;
    std::future<void> readyFuture = ready.get_future();
    qtThread = new std::thread{qtThreadRun, std::move(ready)};
    readyFuture.wait();
}

static void qtThreadStop()
{
    if (!qtThread) {
        return;
    }

    QMetaObject::invokeMethod(qGuiApp, "quit", Qt::QueuedConnection);
    qDebug("%s joining Qt thread", __func__);
    qtThreadJoin();
    qDebug("%s done joining Qt thread", __func__);
}

// TODO how to unregister types?
static void registerQmlTypes()
{
    qmlRegisterType<SessionListModel>("org.wahjam.client", 1, 0, "SessionListModel");
    qmlRegisterType<JamApiManager>("org.wahjam.client", 1, 0, "JamApiManager");
}

void globalInit()
{
    qtThreadStart();

    if (initCount++ == 0) {
        registerQmlTypes();
    }
}

void globalCleanup()
{
    qDebug("%s initcount %d", __func__, initCount);
    if (initCount == 0 || --initCount > 0) {
        return;
    }

    qtThreadStop();

    if (logfp != stderr) {
        fclose(logfp);
        logfp = stderr;
    }
}
