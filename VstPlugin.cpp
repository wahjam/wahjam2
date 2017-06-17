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
        return static_cast<VstPlugin*>(aeffect->user);
    } else {
        return nullptr;
    }
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
    }

    parent = QWindow::fromWinId((WId)(uintptr_t)ptrarg);

    qDebug("%s parent %p threadID %p", __func__, parent, QThread::currentThreadId());

    if (parent) {
        view->setParent(parent);
        view->resize(parent->size());
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
        QSize parentSize{parent->size()};
        QSize viewSize{view->size()};
        if (view->size() != parentSize) {
            qDebug("view->size() %ux%u parent->size() %ux%u",
                   viewSize.width(), viewSize.height(),
                   parentSize.width(), parentSize.height());
            view->resize(parentSize);
        } else {
            view->update();
        }
    }
}

static struct {
    int op;
    const char *name;
} dispatcherOps[] = {
    {0, "effOpen"},
    {1, "effClose"},
    {2, "effSetProgram"},
    {3, "effGetProgram"},
    {5, "effGetProgramName"},
    {8, "effGetParamName"},
    {10, "effSetSampleRate"},
    {11, "effSetBlockSize"},
    {12, "effMainsChanged"},
    {13, "effEditGetRect"},
    {14, "effEditOpen"},
    {15, "effEditClose"},
    {19, "effEditIdle"},
    {20, "effEditTop"},
    {25, "effProcessEvents"},
    {45, "effGetEffectName"},
    {47, "effGetVendorString"},
    {48, "effGetProductString"},
    {49, "effGetVendorVersion"},
    {51, "effCanDo"},
    {56, "effGetParameterProperties"},
    {58, "effGetVstVersion"},
    {-1, nullptr},
};

static void printDispatcher(AEffect *aeffect, int op, int intarg, intptr_t intptrarg, void *ptrarg, float floatarg)
{
    const char *name = "unknown";
    int i;

    for (i = 0; dispatcherOps[i].name; i++) {
        if (op == dispatcherOps[i].op) {
            name = dispatcherOps[i].name;
            break;
        }
    }
    qDebug("vst op %s (%#x) aeffect %p %#08x %p %p %f threadId %p",
           name, op, aeffect, intarg,
           (void*)intptrarg, ptrarg, floatarg,
           QThread::currentThreadId());
}

extern "C" intptr_t dispatcher(AEffect *aeffect, int op, int intarg, intptr_t intptrarg, void *ptrarg, float floatarg)
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

    case effEditTop:
    case effSetSampleRate:
    case effSetBlockSize:
    case effMainsChanged:
    default:
        /* TODO */
        return 0;
    }
}

extern "C" void processReplacing(AEffect *aeffect, float **inbuf, float **outbuf, int ns)
{
    Q_UNUSED(aeffect);
    Q_UNUSED(inbuf);
    Q_UNUSED(outbuf);
    Q_UNUSED(ns);
    /* TODO */
}

VstPlugin::VstPlugin()
    : view{nullptr}, parent{nullptr}
{
    aeffect = {
        .magic              = kEffectMagic,
        .dispatcher         = dispatcher,
        .process            = nullptr,
        .setParameter       = nullptr,
        .getParameter       = nullptr,
        .numPrograms        = 0,
        .numParams          = 0,
        .numInputs          = 2,
        .numOutputs         = 2,
        .flags              = effFlagsHasEditor | effFlagsCanReplacing,
        .ptr1               = nullptr,
        .ptr2               = nullptr,
        .empty3             = {},
        .unkown_float       = 0.f,
        .ptr3               = nullptr,
        .user               = static_cast<void*>(this),
        .uniqueID           = ('W' << 24) | ('J' << 16) | ('A' << 8) | 'M',
        .unknown1           = {},
        .processReplacing   = processReplacing,
    };

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
        editClose();
        view->deleteLater();
        view = nullptr;
    }
}

extern "C" Q_DECL_EXPORT AEffect *VSTPluginMain(audioMasterCallback amc)
{
    Q_UNUSED(amc);

    globalInit();

    qDebug("%s threadId %p", __func__, QThread::currentThreadId());

    VstPlugin *plugin = new VstPlugin;
    plugin->moveToThread(qGuiApp->thread());

    return &plugin->aeffect;
}
