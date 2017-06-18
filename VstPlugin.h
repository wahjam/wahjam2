#pragma once

#include <QQuickView>
#include <QMutex>
#include <QTimer>

#include "aeffectx.h"
#include "AudioProcessor.h"

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

    AEffect aeffect;
    MyRect viewRect;
    QMutex dispatcherMutex;

    static VstPlugin *fromAEffect(AEffect *aeffect);

    VstPlugin();
    ~VstPlugin();

    void setSampleRate(float sampleRate);
    void setRunning(bool enabled);

    // This is called from the real-time thread
    void processReplacing(float **inbuf, float **outbuf, int ns);

public slots:
    void editOpen(void *ptrarg);
    void editClose();
    void editIdle();

private:
    audioMasterCallback audioMasterCallback;
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
    void initializeInQtThread();
    void viewStatusChanged(QQuickView::Status status);
    void periodicTick();
};
