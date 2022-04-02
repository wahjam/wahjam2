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

    QGuiApplication app(argc, argv);

    globalInit();
    QmlGlobals::registerQmlTypes();

    PortAudioEngine portAudioEngine;
    QQmlEngine::setObjectOwnership(&portAudioEngine, QQmlEngine::CppOwnership);
    qmlRegisterSingletonType<PortAudioEngine>("org.wahjam.client", 1, 0,
            "PortAudioEngine",
            [&](QQmlEngine *engine, QJSEngine *) -> QObject * {
        return &portAudioEngine;
    });

    portAudioEngine.logDeviceInfo();

    {
        AppView appView{"standalone"};

        QObject::connect(&portAudioEngine, &PortAudioEngine::runningChanged,
            [&](bool enabled) {
                if (enabled) {
                    appView.setSampleRate(portAudioEngine.sampleRate());
                }
                appView.setAudioRunning(enabled);
            }
        );

        portAudioEngine.setProcessFn(
            [&](float *inOutSamples[CHANNELS_STEREO],
                size_t nsamples,
                SampleTime now) {
                appView.process(inOutSamples, nsamples, now);
            }
        );

        appView.setSource({"qrc:/qml/application.qml"});
        appView.show();

        rc = app.exec();

        portAudioEngine.stop(false);
    }

    globalCleanup();
    return rc;
}
