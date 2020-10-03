// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <QObject>

#include "audio/AudioProcessor.h"

class Metronome : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int bpm MEMBER bpm_ NOTIFY bpmChanged)
    Q_PROPERTY(bool monitor READ monitorEnabled WRITE setMonitorEnabled NOTIFY monitorChanged)

public:
    Metronome(AudioProcessor *processor, QObject *parent = nullptr);
    ~Metronome();

    bool monitorEnabled() const;

signals:
    void bpmChanged(int bpm);
    void monitorChanged(bool monitor);

public slots:
    void start();
    void stop();
    void processAudioStreams();

    // Mute/unmute audio
    void setMonitorEnabled(bool monitor);

private:
    AudioProcessor *processor;
    AudioStream *stream;
    std::vector<float> click;
    int bpm_;
    size_t samplesPerBeat;
    SampleTime startTime;
    SampleTime now;
    bool monitor;
};
