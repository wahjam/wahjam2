// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <QMutex>

#include "aeffectx.h"
#include "core/AppView.h"

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

public slots:
    void editOpen(void *ptrarg);
    void editClose();
    void editIdle();

private:
    audioMasterCallback masterCallback;
    AppView appView;
    QWindow *parent;        // foreign window in host application

    // Current time, in samples.  Only accessed by real-time thread.
    SampleTime now;
};
