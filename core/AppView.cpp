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
    : QQuickView(parent)
{
    // Install Quick error logger
    QObject::connect(this, &QQuickView::statusChanged,
        [=] (QQuickView::Status) { showViewErrors(this); });

    setResizeMode(QQuickView::SizeRootObjectToView);

    // Now load the QML
    setSource(url);
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

    // Invoke immediately to minimize latency
    processAudioStreams();

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

void AppView::process(float *inOutSamples[CHANNELS_STEREO],
                      size_t nsamples,
                      SampleTime now)
{
    processor.process(inOutSamples, nsamples, now);
}
