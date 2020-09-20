// SPDX-License-Identifier: Apache-2.0
#include <stdint.h>
#include <string.h>
#include <QtGlobal>
#include <QGuiApplication>
#include <QThread>
#include <QMutexLocker>

#include "core/global.h"
#include "VstPlugin.h"

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

static void qtThreadRun(std::promise<void> ready)
{
    /*
     * Qt relies on this data remaining accessible across subsequent
     * QCoreApplication singleton instances so make it static.
     */
    static char progname[] = {'V', 'S', 'T', '\0'};
    static char *argv = progname;
    static int argc = 1;

    installMessageHandler();

    QGuiApplication app{argc, &argv};

    globalInit();

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

VstPlugin *VstPlugin::fromAEffect(AEffect *aeffect)
{
    if (aeffect) {
        return static_cast<VstPlugin*>(aeffect->ptr3);
    } else {
        return nullptr;
    }
}

void VstPlugin::setSampleRate(float sampleRate)
{
    appView.setSampleRate(sampleRate);
}

void VstPlugin::setRunning(bool enabled)
{
    now = 0;
    appView.setAudioRunning(enabled);
}

void VstPlugin::editOpen(void *ptrarg)
{
    if (parent) {
        qDebug("%s called with editor already open", __func__);
        return;
    }

    parent = QWindow::fromWinId((WId)(uintptr_t)ptrarg);

    qDebug("%s parent %p threadID %p", __func__, parent, QThread::currentThreadId());

    if (parent) {
        /*
         * Calling setParent(nullptr) adds a window frame. Frame margins are
         * not reset again by QWindow when reparenting to a real parent. This
         * causes incorrect geometry calculations since non-existent margins
         * will be included. Turn off the window frame.
         */
        appView.setFlags(appView.flags() | Qt::FramelessWindowHint);

        appView.setParent(parent);
    }

    appView.resize(viewRect.right, viewRect.bottom);
    appView.show();
}

void VstPlugin::editClose()
{
    qDebug("%s parent %p", __func__, parent);

    appView.hide();
    appView.setParent(nullptr);

    if (parent) {
        // See editOpen()
        appView.setFlags(appView.flags() & ~Qt::FramelessWindowHint);

        parent->deleteLater();
        parent = nullptr;
    }
}

void VstPlugin::editIdle()
{
    appView.update();
}

static struct {
    int op;
    const char *name;
    bool print;
} dispatcherOps[] = {
    {0, "effOpen", true},
    {1, "effClose", true},
    {2, "effSetProgram", true},
    {3, "effGetProgram", true},
    {5, "effGetProgramName", true},
    {8, "effGetParamName", true},
    {10, "effSetSampleRate", true},
    {11, "effSetBlockSize", true},
    {12, "effMainsChanged", true},
    {13, "effEditGetRect", false},
    {14, "effEditOpen", true},
    {15, "effEditClose", true},
    {19, "effEditIdle", false},
    {20, "effEditTop", true},
    {25, "effProcessEvents", true},
    {45, "effGetEffectName", true},
    {47, "effGetVendorString", true},
    {48, "effGetProductString", true},
    {49, "effGetVendorVersion", true},
    {51, "effCanDo", true},
    {53, "unknown", false},
    {56, "effGetParameterProperties", true},
    {58, "effGetVstVersion", true},
    {-1, nullptr, false},
};

static void printDispatcher(AEffect *aeffect, int op, int intarg, intptr_t intptrarg, void *ptrarg, float floatarg)
{
    const char *name = "unknown";
    int i;

    for (i = 0; dispatcherOps[i].name; i++) {
        if (op == dispatcherOps[i].op) {
            if (!dispatcherOps[i].print) {
                return;
            }
            name = dispatcherOps[i].name;
            break;
        }
    }
    qDebug("vst op %s (%#x) aeffect %p %#08x %p %p %f threadId %p",
           name, op, aeffect, intarg,
           (void*)intptrarg, ptrarg, floatarg,
           QThread::currentThreadId());
}

static intptr_t dispatcher(AEffect *aeffect, int op, int intarg, intptr_t intptrarg, void *ptrarg, float floatarg)
{
    printDispatcher(aeffect, op, intarg, intptrarg, ptrarg, floatarg);

    /*
     * dispatcher() is typically invoked from the host GUI thread.  The Qt GUI
     * runs in our Qt thread.  QMetaObject::invoke() is used to synchronize
     * with the Qt thread.
     *
     * The host GUI thread is unable to process Windows messages until
     * dispatcher() returns.  Do not make blocking calls from dispatcher() to
     * the Qt thread with
     * QMetaObject::invokeMethod(Qt::BlockingQueuedConnection) or similar
     * because any SendMessage() calls from the Qt thread to the host GUI
     * thread will then deadlock!
     */
    VstPlugin *plugin = VstPlugin::fromAEffect(aeffect);
    if (!plugin) {
        qDebug("invalid aeffect, cannot get plugin instance");
        return 0;
    }

    // Prevent re-entrancy and make deadlocks obvious.
    QMutexLocker locker{&plugin->dispatcherMutex};

    switch (op) {
    case effGetEffectName:
        snprintf((char *)ptrarg, 32, "Wahjam2");
        return 1;

    case effGetVendorString:
        snprintf((char *)ptrarg, 32, "The Wahjam Project");
        return 1;

    case effGetProductString:
        snprintf((char *)ptrarg, 32, "Wahjam2");
        return 1;

    case effGetVstVersion:
        return 2;

    case effOpen:
        return 0; /* do nothing */

    case effClose:
        locker.unlock(); // can't leave mutex locked while plugin is deleted!
        QMetaObject::invokeMethod(plugin, "deleteLater", Qt::QueuedConnection);

        qDebug("%s initcount %d", __func__, initCount);
        if (initCount > 0 && --initCount == 0) {
            qtThreadStop();
            globalCleanup();
        }
        return 0;

    case effCanDo:
        qDebug("%s", (char *)ptrarg);
        return 0;

    case effEditGetRect:
        *static_cast<VstPlugin::MyRect**>(ptrarg) = &plugin->viewRect;
        return 1;

    case effEditOpen:
        if (!QMetaObject::invokeMethod(plugin, "editOpen",
                                       Qt::QueuedConnection,
                                       Q_ARG(void*, ptrarg))) {
            qDebug("invokeMethod editOpen failed");
        }
        return 1;

    case effEditClose:
        if (!QMetaObject::invokeMethod(plugin, "editClose",
                                       Qt::QueuedConnection)) {
            qDebug("invokeMethod editClose failed");
        }
        return 0;

    case effEditIdle:
        if (!QMetaObject::invokeMethod(plugin, "editIdle",
                                       Qt::QueuedConnection)) {
            qDebug("invokeMethod editIdle failed");
        }
        return 0;

    case effSetSampleRate:
        // Call directly, this doesn't use the Qt thread and must be synchronous
        plugin->setSampleRate(floatarg);
        return 0;

    case effMainsChanged:
        // Call directly, this doesn't use the Qt thread and must be synchronous
        plugin->setRunning(intptrarg);
        return 0;

    default:
        /* TODO */
        return 0;
    }
}

void VstPlugin::processReplacing(float **inbuf, float **outbuf, int ns)
{
    // Initialize outbuf in case we weren't invoked in replacing fashion
    for (int ch = 0; ch < CHANNELS_STEREO; ch++) {
        if (inbuf[ch] != outbuf[ch]) {
            memcpy(outbuf[ch], inbuf[ch], ns * sizeof(float));
        }
    }

    // Fetch current sample time from host
    intptr_t result = masterCallback(&aeffect, audioMasterGetTime,
                                     0, 0, nullptr, 0.f);
    VstTimeInfo *info = reinterpret_cast<VstTimeInfo*>(result);
    if (info) {
        SampleTime samplePos = info->samplePos + 0.5;
        if (samplePos != now) {
            qDebug("jump in samplePos.  expected %llu, got %llu (info->samplePos %f %#llx)",
                   (long long unsigned)now,
                   (long long unsigned)samplePos,
                   info->samplePos,
                   *(long long unsigned*)&info->samplePos);
        }
        now = samplePos;
    }

    appView.process(outbuf, ns, now);

    now += ns;
}

static void processReplacingCallback(AEffect *aeffect, float **inbuf, float **outbuf, int ns)
{
    VstPlugin *plugin = VstPlugin::fromAEffect(aeffect);
    if (!plugin) {
        return;
    }

    plugin->processReplacing(inbuf, outbuf, ns);
}

VstPlugin::VstPlugin(audioMasterCallback masterCallback_)
    : masterCallback{masterCallback_},
      appView{QUrl{"qrc:/qml/application.qml"}},
      parent{nullptr}
{
    memset(&aeffect, 0, sizeof(aeffect));
    aeffect.magic               = kEffectMagic;
    aeffect.dispatcher          = dispatcher;
    aeffect.numInputs           = 2;
    aeffect.numOutputs          = 2;
    aeffect.flags               = effFlagsHasEditor | effFlagsCanReplacing;
    aeffect.ptr3                = static_cast<void*>(this);
    aeffect.uniqueID            = ('W' << 24) | ('J' << 16) | ('A' << 8) | 'M';
    aeffect.processReplacing    = processReplacingCallback;

    viewRect = {
        .top = 0,
        .left = 0,
        .bottom = 600,
        .right = 800,
    };
}

VstPlugin::~VstPlugin()
{
    qDebug("%s", __func__);

    appView.hide();
    appView.setParent(nullptr);

    if (parent) {
        delete parent;
        parent = nullptr;
    }
}

extern "C" Q_DECL_EXPORT AEffect *VSTPluginMain(audioMasterCallback masterCallback)
{
    qtThreadStart();

    if (initCount++ == 0) {
        registerQmlTypes();
    }

    qDebug("%s threadId %p", __func__, QThread::currentThreadId());

    VstPlugin *plugin = new VstPlugin{masterCallback};
    plugin->moveToThread(qGuiApp->thread());

    return &plugin->aeffect;
}
