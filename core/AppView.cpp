// SPDX-License-Identifier: Apache-2.0
#include <inttypes.h>
#include <QQmlError>
#include "config.h"
#include "AppView.h"
#include "QmlGlobals.h"

static void showViewErrors(QQuickView *view)
{
    for (const QQmlError &error : view->errors()) {
        qCritical("%s", error.toString().toLatin1().constData());
    }
}

AppView::AppView(const QString &format, const QUrl &url, QWindow *parent)
    : QQuickView{parent}, transportResetPending{false}
{
    qmlGlobals_ = new QmlGlobals{this, format};

    // Install Quick error logger
    QObject::connect(this, &QQuickView::statusChanged,
        [this] (QQuickView::Status) { showViewErrors(this); });

    setTitle(APPNAME);
    setResizeMode(QQuickView::SizeRootObjectToView);
    setMinimumSize(QSize{800, 600});

    // TODO make SSL verification a boolean setting
    qWarning("SSL verification disabled for development");
    QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
    sslConfig.setPeerVerifyMode(QSslSocket::QueryPeer);
    QSslConfiguration::setDefaultConfiguration(sslConfig);

    // Now load the QML
    setSource(url);
}

AppView::~AppView()
{
    // Delete qmlGlobals now so audio streams are destroyed before processor
    delete qmlGlobals_;
}

void AppView::processAudioStreamsTick()
{
    // Warn if timer jitter might cause performance problems
    qint64 now = audioRunningTimer.nsecsElapsed();
    if (lastProcessAudioStreamsTick != 0) {
        int64_t duration = now - lastProcessAudioStreamsTick;
        if (duration > 2 * SAFE_PERIODIC_TICK_MSEC * 1000 * 1000) {
            qWarning("%s called %" PRId64 " ns after last time", __func__, duration);
        }
    }
    lastProcessAudioStreamsTick = now;

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

    lastProcessAudioStreamsTick = 0;
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

SampleTime AppView::currentSampleTime() const
{
    const SampleTime nsecsPerSecond = 1000000000;
    return audioRunningTimer.nsecsElapsed() *
           processor.getSampleRate() /
           nsecsPerSecond;
}

// May be called from another thread
void AppView::setAudioRunning(bool enabled)
{
    QMutexLocker locker{&processorWriteLock};

    audioRunningTimer.start();
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
    audioRunningTimer.start();
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
