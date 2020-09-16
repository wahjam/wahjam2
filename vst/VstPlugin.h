// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <QQuickView>
#include <QMutex>
#include <QTimer>

#include "aeffectx.h"
#include "audio/AudioProcessor.h"

class VstPlugin : public QObject
{
    Q_OBJECT

public:
    struct MyRect {
        short top;
        short left;
        short bottom;
        short right;
    };

    // Internals - only access from VST dispatcher
    AEffect aeffect;
    MyRect viewRect;
    QMutex dispatcherMutex;

    // Internals - only call from VST dispatcher
    static VstPlugin *fromAEffect(AEffect *aeffect);
    void setSampleRate(float sampleRate);
    void setRunning(bool enabled);

    VstPlugin(audioMasterCallback audioMasterCallback);
    ~VstPlugin();

    // This is called from the real-time thread
    void processReplacing(float **inbuf, float **outbuf, int ns);

signals:
    // Emitted periodically to allow draining capture streams and refilling
    // playback streams.
    void processAudioStreams();

public slots:
    void editOpen(void *ptrarg);
    void editClose();
    void editIdle();

private:
    audioMasterCallback masterCallback;
    QQuickView *view;
    QWindow *parent;        // foreign window in host application
    AudioProcessor processor;

    // Current time, in samples.  Only accessed by real-time thread.
    SampleTime now;

    // This mutex ensures that timer callbacks do not race with dispatcher
    // calls (e.g. effMainsChanged)
    QMutex processorWriteLock;

    QTimer *periodicTimer;

private slots:
    void periodicTick();
    void startPeriodicTick();
    void stopPeriodicTick();
    void viewStatusChanged(QQuickView::Status status);
};
