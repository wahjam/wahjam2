// SPDX-License-Identifier: Apache-2.0
#include <QGuiApplication>

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

    {
        AppView appView{QUrl{"qrc:/qml/application.qml"}};

        PortAudioEngine portAudioEngine{
            [&] (float *inOutSamples[CHANNELS_STEREO],
                 size_t nsamples,
                 SampleTime now) {
                appView.process(inOutSamples, nsamples, now);
            }
        };

        portAudioEngine.logDeviceInfo();

        portAudioEngine.setHostApi("ALSA");
        portAudioEngine.setInputDevice("Scarlett 2i4 USB: Audio (hw:3,0)");
        portAudioEngine.setInputRouting({
            ChannelRoute::LEFT,
            ChannelRoute::RIGHT,
        });
        portAudioEngine.setOutputDevice("Scarlett 2i4 USB: Audio (hw:3,0)");
        portAudioEngine.setOutputRouting({
            ChannelRoute::LEFT,
            ChannelRoute::RIGHT,
            ChannelRoute::OFF,
            ChannelRoute::OFF,
        });

        if (!portAudioEngine.start()) {
            qFatal("Failed to start audio");
        }

        appView.setSampleRate(portAudioEngine.sampleRate());
        appView.setAudioRunning(true);
        appView.show();

        rc = app.exec();

        appView.setAudioRunning(false);
        portAudioEngine.stop();
    }

    globalCleanup();
    return rc;
}
