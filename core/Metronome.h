// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <QObject>

#include "audio/AudioProcessor.h"

class Metronome : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int bpm MEMBER bpm_ NOTIFY bpmChanged)

public:
    Metronome(AudioProcessor *processor, QObject *parent = nullptr);
    ~Metronome();

signals:
    void bpmChanged(int bpm);

public slots:
    void start();
    void stop();
    void processAudioStreams();

private:
    AudioProcessor *processor;
    AudioStream *stream;
    std::vector<float> click;
    int bpm_;
    size_t samplesPerBeat;
    SampleTime startTime;
    SampleTime now;
};
