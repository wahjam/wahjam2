// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <QMutex>
#include <QQuickView>
#include <QTimer>
#include <QElapsedTimer>
#include "audio/AudioProcessor.h"

// Don't #include it because of the circular dependency on AppView
class QmlGlobals;

// The application including the user interface and audio.
class AppView : public QQuickView
{
    Q_OBJECT

public:
    AppView(const QString &format, const QUrl &url = {}, QWindow *parent = nullptr);
    ~AppView();

    // Needed by QmlGlobals::registerQmlTypes()
    QmlGlobals *qmlGlobals()
    {
        return qmlGlobals_;
    }

    AudioProcessor *audioProcessor()
    {
        return &processor;
    }

    // Can be called from any thread
    void setSampleRate(int sampleRate) {
        processor.setSampleRate(sampleRate);
    }

    // Returns calculated sample position. Called from the Qt thread.
    SampleTime currentSampleTime() const;

    // Process audio samples. Called from the real-time audio thread.
    void process(float *inOutSamples[CHANNELS_STEREO],
                 size_t nsamples,
                 SampleTime now);

public slots:
    // Start/stop audio processing, can be called from any thread
    void setAudioRunning(bool enabled);

signals:
    // Emitted periodically to allow draining capture streams and refilling
    // playback streams.
    void processAudioStreams();

private slots:
    void processAudioStreamsTick();
    void startProcessAudioStreamsTimer();
    void stopProcessAudioStreamsTimer();
    void transportReset();

private:
    AudioProcessor processor;
    std::atomic<bool> transportResetPending;

    // For currentSampleTime()
    QElapsedTimer audioRunningTimer;

    // Protects setAudioRunning() vs processAudioStreamsTick()
    QMutex processorWriteLock;

    QTimer processAudioStreamsTimer;

    QmlGlobals *qmlGlobals_;
};
