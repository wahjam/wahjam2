#include <thread>
#include <future>
#include <QGuiApplication>
#include <QQmlEngine>
#include <QThread>

#include "global.h"
#include "SessionListModel.h"

// How many instances have initialized us?
static unsigned int initCount;

/*
 * VST plugins have no periodic callback suitable for Qt event loop
 * integration.  effEditIdle is only called while the editor GUI is open and
 * therefore cannot be used.
 *
 * It is necessary to run a dedicated Qt GUI thread.
 */
static std::thread *qtThread;

static FILE *logfp;

static void qtMessageHandler(QtMsgType type,
                             const QMessageLogContext &context,
                             const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();

    Q_UNUSED(context);

    if (logfp) {
        fprintf(logfp, "%s\n", localMsg.constData());
    }
    if (type == QtFatalMsg) {
        abort();
    }
}

static void qtThreadRun(std::promise<void> ready)
{
    char progname[] = {'V', 'S', 'T', '\0'};
    char *argv = progname;
    int argc = 1;

    qInstallMessageHandler(qtMessageHandler);

    QGuiApplication app{argc, &argv};
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
    if (qtThread) {
        return;
    }

    std::promise<void> ready;
    std::future<void> readyFuture = ready.get_future();
    qtThread = new std::thread{qtThreadRun, std::move(ready)};
    readyFuture.wait();
}

void globalInit()
{
    if (!logfp) {
        logfp = fopen("/tmp/vst.log", "w");
    }

    if (!qGuiApp) {
        qtThreadStart();
        qmlRegisterType<SessionListModel>("com.aucalic.client", 1, 0, "SessionListModel");
    }

    initCount++;
}

void globalCleanup()
{
    qDebug("%s initcount %d", __func__, initCount);
    if (initCount == 0 || --initCount > 0) {
        return;
    }

    QMetaObject::invokeMethod(qGuiApp, "quit",
                              Qt::QueuedConnection);
    qDebug("%s joining Qt thread", __func__);
    qtThreadJoin();
    qDebug("%s done joining Qt thread", __func__);

    if (logfp) {
        fclose(logfp);
        logfp = nullptr;
    }
}
