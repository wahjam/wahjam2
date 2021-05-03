// SPDX-License-Identifier: Apache-2.0
#include <QGuiApplication>
#include <QQmlEngine>

#include "core/global.h"
#include "core/AppView.h"
#include "core/QmlGlobals.h"
#include "PortAudioEngine.h"

int main(int argc, char **argv)
{
    int rc;

    installMessageHandler();

    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication app(argc, argv);

    globalInit();
    QmlGlobals::registerQmlTypes();

    PortAudioEngine portAudioEngine;
    QQmlEngine::setObjectOwnership(&portAudioEngine, QQmlEngine::CppOwnership);
    qmlRegisterSingletonType<QmlGlobals>("org.wahjam.client", 1, 0, "PortAudioEngine",
            [&](QQmlEngine *engine, QJSEngine *) -> QObject * {
        return &portAudioEngine;
    });

    portAudioEngine.logDeviceInfo();

    {
        AppView appView{"standalone", QUrl{"qrc:/qml/application.qml"}};

        portAudioEngine.setProcessFn(
            [&](float *inOutSamples[CHANNELS_STEREO],
                size_t nsamples,
                SampleTime now) {
                appView.process(inOutSamples, nsamples, now);
            }
        );

        appView.show();

        rc = app.exec();

        appView.setAudioRunning(false);
        portAudioEngine.stop();
    }

    globalCleanup();
    return rc;
}
