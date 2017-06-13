#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <QtGlobal>
#include <QGuiApplication>
#include <QQuickView>
#include <QQmlError>

#include "aeffectx.h"
#include "SessionListModel.h"

static FILE *logfp;

struct MyRect {
    short top;
    short left;
    short bottom;
    short right;
};

static MyRect myrect = {
    .top = 0,
    .left = 0,
    .bottom = 600,
    .right = 800,
};

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

    switch (op) {
    case effGetEffectName:
        snprintf((char *)ptrarg, 32, "vsttest");
        return 1;

    case effGetVendorString:
        snprintf((char *)ptrarg, 32, "vendor");
        return 1;

    case effGetProductString:
        snprintf((char *)ptrarg, 32, "product");
        return 1;

    case effGetVstVersion:
        return 2;

    case effOpen:
        return 0; /* do nothing */

    case effClose:
        return 0; /* do nothing */

    case effCanDo:
        fprintf(logfp, "%s\n", (char *)ptrarg);
        return 0;

    case effEditGetRect:
    {
        *(struct MyRect **)ptrarg = &myrect;
        return 1;
    }

    case effEditOpen:
    {
        /* HWND ptrarg */
        QWindow *parent = QWindow::fromWinId((WId)(uintptr_t)ptrarg);
        fprintf(logfp, "parent %p\n", parent);

        QQuickView *view = new QQuickView(parent);
        view->setResizeMode(QQuickView::SizeRootObjectToView);
        view->resize(parent->size());
        view->setSource(QUrl::fromLocalFile("application.qml"));
        view->show();

        QList<QQmlError> errors = view->errors();
        for (int i = 0; i < errors.size(); i++) {
            fprintf(logfp, "Error: %s\n", errors.at(i).toString().toUtf8().constData());
        }
        fprintf(logfp, "Done printing errors\n");

        return 1;
    }

    case effEditIdle:
        QGuiApplication::processEvents();
        qApp->sendPostedEvents(0, -1);
        return 0;

    case effEditClose:
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

extern "C" Q_DECL_EXPORT AEffect *VSTPluginMain(audioMasterCallback amc)
{
    static AEffect aeffect = {
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
        .user               = NULL,
        .uniqueID           = ('J' << 24) | ('N' << 16) | ('E' << 8) | 'T',
        .unknown1           = {},
        .processReplacing   = processReplacing,
    };

    Q_UNUSED(amc);

    if (!qApp) {
        /* This must be mutable and live longer than QApplication */
        static char progname[] = {'v', 's', 't', 't', 'e', 's', 't', '\0'};
        static char *argv = progname;
        static int argc = 1;

        new QGuiApplication(argc, &argv); /* Qt manages singleton instance via qApp */

        qmlRegisterType<SessionListModel>("com.aucalic.client", 1, 0, "SessionListModel");
    }

    if (!logfp) {
        logfp = fopen("/tmp/vst.log", "w");
    }

    fprintf(logfp, "%s\n", __func__);

    return &aeffect;
}
