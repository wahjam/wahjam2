#include <QGuiApplication>
#include <QQmlEngine>

#include "global.h"
#include "SessionListModel.h"

FILE *logfp;

static unsigned int initCount;

bool globalInit()
{
    if (!logfp) {
        logfp = fopen("/tmp/vst.log", "w");
        if (!logfp) {
            return false;
        }
    }

    if (!qApp) {
        // This must be mutable and live longer than QApplication
        static char progname[] = {'V', 'S', 'T', '\0'};
        static char *argv = progname;
        static int argc = 1;

        new QGuiApplication(argc, &argv); // Qt manages singleton instance via qApp

        qmlRegisterType<SessionListModel>("com.aucalic.client", 1, 0, "SessionListModel");
    }

    initCount++;
    return true;
}

void globalCleanup()
{
    if (initCount == 0 || --initCount > 0) {
        return;
    }

    qApp->quit();
    delete qApp;

    fclose(logfp);
    logfp = nullptr;
}
