#include <stdint.h>
#include <string.h>
#include <QtGlobal>
#include <QQmlError>
#include <QGuiApplication>
#include <QThread>
#include <QMutexLocker>

#include "global.h"
#include "VstPlugin.h"

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
    processor.setSampleRate(sampleRate);
}

// Called by an interval timer for Qt thread housekeeping
void VstPlugin::periodicTick()
{
    if (!processor.isRunning()) {
        return;
    }

    QMutexLocker locker{&processorWriteLock};

    emit processAudioStreams();

    processor.tick();
}

// Called from Qt thread
void VstPlugin::startPeriodicTick()
{
    if (periodicTimer) {
        return;
    }

    // setRunning(false) may have been called before our slot was invoked
    if (!processor.isRunning()) {
        return;
    }

    // Invoke immediately to minimize latency
    periodicTick();

    periodicTimer = new QTimer{this};
    connect(periodicTimer, SIGNAL(timeout()),
            this, SLOT(periodicTick()));
    periodicTimer->start(50);
}

// Called from Qt thread
void VstPlugin::stopPeriodicTick()
{
    // setRunning(true) may have been called before our slot was invoked
    if (processor.isRunning()) {
        return;
    }

    delete periodicTimer;
    periodicTimer = nullptr;
}

void VstPlugin::setRunning(bool enabled)
{
    QMutexLocker locker{&processorWriteLock};

    now = 0;
    processor.setRunning(enabled);

    QMetaObject::invokeMethod(this,
            enabled ? "startPeriodicTick" : "stopPeriodicTick",
            Qt::QueuedConnection);
}

void VstPlugin::viewStatusChanged(QQuickView::Status status)
{
    Q_UNUSED(status);

    for (const QQmlError &error : view->errors()) {
        qDebug("QML error: %s", error.toString().toUtf8().constData());
    }
}

void VstPlugin::editOpen(void *ptrarg)
{
    if (parent) {
        qDebug("%s called with editor already open", __func__);
        return;
    }

    if (!view) {
        view = new QQuickView{QUrl{"qrc:/application.qml"}};
        connect(view, SIGNAL(statusChanged(QQuickView::Status)),
                this, SLOT(viewStatusChanged(QQuickView::Status)));

        /*
         * Calling setParent(nullptr) adds a window frame.  Frame margins are
         * not reset again by QWindow when reparenting to a real parent.  This
         * causes incorrect geometry calculations since non-existent margins
         * will be included.  Turn off the window frame.
         */
        view->setFlags(view->flags() | Qt::FramelessWindowHint);

        view->setResizeMode(QQuickView::SizeRootObjectToView);
    }

    parent = QWindow::fromWinId((WId)(uintptr_t)ptrarg);

    qDebug("%s parent %p threadID %p", __func__, parent, QThread::currentThreadId());

    if (parent) {
        view->setParent(parent);
        view->resize(viewRect.right, viewRect.bottom);
        view->show();
    }
}

void VstPlugin::editClose()
{
    qDebug("%s view %p parent %p", __func__, view, parent);

    if (!view) {
        qDebug("%s called with editor already deleted", __func__);
        return;
    }

    if (!parent) {
        return;
    }

    view->hide();
    view->setParent(nullptr);
    parent->deleteLater();
    parent = nullptr;
}

void VstPlugin::editIdle()
{
    if (view) {
        view->update();
    }
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
        globalCleanup();
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

    processor.process(outbuf, ns, now);

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
    : masterCallback{masterCallback_}, view{nullptr},
      parent{nullptr}, periodicTimer{nullptr}
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

    if (view) {
        delete view;
        view = nullptr;
        delete parent;
        parent = nullptr;
    }

    if (periodicTimer) {
        delete periodicTimer;
        periodicTimer = nullptr;
    }
}

extern "C" Q_DECL_EXPORT AEffect *VSTPluginMain(audioMasterCallback masterCallback)
{
    globalInit();

    qDebug("%s threadId %p", __func__, QThread::currentThreadId());

    VstPlugin *plugin = new VstPlugin{masterCallback};
    plugin->moveToThread(qGuiApp->thread());

    return &plugin->aeffect;
}
