#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <QtGlobal>
#include <QApplication>
#include "aeffectx.h"
#include "TestWidget.h"

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
    for (int i = 0; dispatcherOps[i].name; i++) {
        if (op == dispatcherOps[i].op) {
            fprintf(logfp, "dispatch aeffect %p op %s\n", aeffect, dispatcherOps[i].name);
            return;
        }
    }
    fprintf(logfp, "dispatch aeffect %p op %#x\n", aeffect, op);
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
        if (!qApp) {
            int argc = 0;
            new QApplication(argc, NULL);
        }

        TestWidget *w = new TestWidget((HWND)ptrarg);
        w->move(0, 0);
        w->adjustSize();
        w->setMinimumSize(480, 60);
        w->show();
        return 1;
    }

    case effEditIdle:
        QApplication::processEvents();
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
    /* TODO */
}

extern "C" void processReplacing(AEffect *aeffect, float **inbuf, float **outbuf, int ns)
{
    /* TODO */
}

static AEffect aeffect = {
    .magic              = kEffectMagic,
    .dispatcher         = dispatcher,
    .process            = process,
    .numInputs          = 2,
    .numOutputs         = 2,
    .flags              = effFlagsHasEditor | effFlagsCanReplacing,
    .processReplacing   = processReplacing;
};

extern "C" Q_DECL_EXPORT AEffect *VSTPluginMain(audioMasterCallback amc)
{
    if (!logfp) {
        logfp = fopen("/tmp/vst.log", "w");
    }

    fprintf(logfp, "%s\n", __func__);

    return &aeffect;
}
