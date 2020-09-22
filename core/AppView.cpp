// SPDX-License-Identifier: Apache-2.0
#include <QQmlError>
#include "AppView.h"

static void showViewErrors(QQuickView *view)
{
    for (const QQmlError &error : view->errors()) {
        qCritical("%s", error.toString().toLatin1().constData());
    }
}

AppView::AppView(const QUrl &url, QWindow *parent)
    : QQuickView{parent}, transportResetPending{false}, metronome{&processor}
{
    // Install Quick error logger
    QObject::connect(this, &QQuickView::statusChanged,
        [=] (QQuickView::Status) { showViewErrors(this); });

    setResizeMode(QQuickView::SizeRootObjectToView);

    // Now load the QML
    setSource(url);

    connect(this, &AppView::processAudioStreams,
            &metronome, &Metronome::processAudioStreams);
    metronome.start();
}

void AppView::processAudioStreamsTick()
{
    if (!processor.isRunning()) {
        return;
    }

    // So that set setRunning can be called from another thread
    QMutexLocker locker{&processorWriteLock};

    emit processAudioStreams();

    processor.tick();
}

void AppView::startProcessAudioStreamsTimer()
{
    if (processAudioStreamsTimer.isActive()) {
        return;
    }

    // setRunning(false) may have been called before our slot was invoked
    if (!processor.isRunning()) {
        return;
    }

    // Emit processAudioStreams() immediately to minimize latency
    {
        QMutexLocker locker{&processorWriteLock};
        emit processAudioStreams();
    }

    connect(&processAudioStreamsTimer, &QTimer::timeout,
            this, &AppView::processAudioStreamsTick);
    processAudioStreamsTimer.start(SAFE_PERIODIC_TICK_MSEC);
}

void AppView::stopProcessAudioStreamsTimer()
{
    // setRunning(true) may have been called before our slot was invoked
    if (processor.isRunning()) {
        return;
    }

    processAudioStreamsTimer.stop();
}

// May be called from another thread
void AppView::setAudioRunning(bool enabled)
{
    QMutexLocker locker{&processorWriteLock};

    processor.setRunning(enabled);

    QMetaObject::invokeMethod(this,
            enabled ? "startProcessAudioStreamsTimer" :
                      "stopProcessAudioStreamsTimer",
            Qt::QueuedConnection);
}

// The part of transport reset that runs in the Qt thread
void AppView::transportReset()
{
    QMutexLocker locker{&processorWriteLock};

    processor.setRunning(false);
    processor.setRunning(true);
    emit processAudioStreams();
    transportResetPending.store(false);
}

void AppView::process(float *inOutSamples[CHANNELS_STEREO],
                      size_t nsamples,
                      SampleTime now)
{
    // Detect when time is reset. This is complicated by the fact that
    // transportReset() needs to run in the Qt thread. We'll silence output
    // while the transport is being reset.
    bool resetPending = transportResetPending.load();
    bool needsReset = false;

    if (!resetPending) {
        needsReset = now < processor.getNextSampleTime();

        // This tells the processor the new 'now' time value, so do it even
        // when reset is needed.
        processor.process(inOutSamples, nsamples, now);
    }

    if (needsReset) {
        transportResetPending.store(true);
        QMetaObject::invokeMethod(this, "transportReset",
                                  Qt::QueuedConnection);
    }

    // Silence output during transport reset
    if (resetPending || needsReset) {
        for (int ch = 0; ch < CHANNELS_STEREO; ch++) {
            for (size_t i = 0; i < nsamples; i++) {
                inOutSamples[ch][i] = 0.f;
            }
        }
    }
}
