#include <stdint.h>
#include <string.h>
#include <QtGlobal>
#include <QQmlError>
#include <QGuiApplication>

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

int VstPlugin::editOpen(WId parentId)
{
    if (view) {
        view->show();
        return 1;
    }

    QWindow *parent = QWindow::fromWinId(parentId);
    fprintf(logfp, "parent %p\n", parent);

    view = new QQuickView{parent};
    view->setResizeMode(QQuickView::SizeRootObjectToView);
    view->resize(parent->size());
    view->setSource(QUrl::fromLocalFile("application.qml"));
    view->show();

    QList<QQmlError> errors = view->errors();
    for (int i = 0; i < errors.size(); i++) {
        fprintf(logfp, "Error: %s\n", errors.at(i).toString().toUtf8().constData());
    }
    fprintf(logfp, "Done printing errors\n");
    if (errors.size()) {
        delete view;
        view = nullptr;
        return 0;
    }

    return 1;
}

void VstPlugin::editClose()
{
    if (view) {
        view->deleteLater();
        view = nullptr;
    }
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
    {-1, NULL},
};

static void printDispatcher(AEffect *aeffect, int op, int intarg, intptr_t intptrarg, void *ptrarg, float floatarg)
{
    int i;

    for (i = 0; dispatcherOps[i].name; i++) {
        if (op == dispatcherOps[i].op) {
            fprintf(logfp, "dispatch aeffect %p op %s ", aeffect, dispatcherOps[i].name);
            break;
        }
    }
    if (!dispatcherOps[i].name) {
        fprintf(logfp, "dispatch aeffect %p op %#x ", aeffect, op);
    }
    fprintf(logfp, "%#08x %p %p %f\n", intarg, (void*)intptrarg, ptrarg, floatarg);
}

extern "C" intptr_t dispatcher(AEffect *aeffect, int op, int intarg, intptr_t intptrarg, void *ptrarg, float floatarg)
{
    printDispatcher(aeffect, op, intarg, intptrarg, ptrarg, floatarg);

    VstPlugin *plugin = VstPlugin::fromAEffect(aeffect);
    if (!plugin) {
        fprintf(logfp, "invalid aeffect, cannot get plugin instance\n");
        return 0;
    }

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
        delete plugin;
        globalCleanup();
        return 0;

    case effCanDo:
        fprintf(logfp, "%s\n", (char *)ptrarg);
        return 0;

    case effEditGetRect:
        *static_cast<VstPlugin::MyRect**>(ptrarg) = &plugin->viewRect;
        return 1;

    case effEditOpen:
        return plugin->editOpen((WId)(uintptr_t)ptrarg);

    case effEditClose:
        plugin->editClose();
        return 0;

    case effEditIdle:
        plugin->editIdle();
        QGuiApplication::processEvents();
        qApp->sendPostedEvents(0, -1);
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

extern "C" void process(AEffect *aeffect, float **inbuf, float **outbuf, int ns)
{
    Q_UNUSED(aeffect);
    Q_UNUSED(inbuf);
    Q_UNUSED(outbuf);
    Q_UNUSED(ns);
    /* TODO */
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
    : view{nullptr}
{
    aeffect = {
        .magic              = kEffectMagic,
        .dispatcher         = dispatcher,
        .process            = process,
        .setParameter       = NULL,
        .getParameter       = NULL,
        .numPrograms        = 0,
        .numParams          = 0,
        .numInputs          = 2,
        .numOutputs         = 2,
        .flags              = effFlagsHasEditor | effFlagsCanReplacing,
        .ptr1               = NULL,
        .ptr2               = NULL,
        .empty3             = {},
        .unkown_float       = 0.f,
        .ptr3               = NULL,
        .user               = static_cast<void*>(this),
        .uniqueID           = ('A' << 24) | ('U' << 16) | ('C' << 8) | 'A',
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
    editClose();

    // Run event loop on final time
    QGuiApplication::processEvents();
    qApp->sendPostedEvents(0, -1);
}

extern "C" Q_DECL_EXPORT AEffect *VSTPluginMain(audioMasterCallback amc)
{
    Q_UNUSED(amc);

    if (!globalInit()) {
        return nullptr;
    }

    fprintf(logfp, "%s\n", __func__);

    VstPlugin *plugin = new VstPlugin;
    return &plugin->aeffect;
}
