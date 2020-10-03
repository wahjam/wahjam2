// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <QTimer>

#include "audio/AudioProcessor.h"

class Metronome : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int beat MEMBER beat_ NOTIFY beatChanged)
    Q_PROPERTY(int bpm MEMBER bpm_ NOTIFY bpmChanged)
    Q_PROPERTY(int bpi MEMBER bpi_ NOTIFY bpiChanged)
    Q_PROPERTY(bool monitor READ monitorEnabled WRITE setMonitorEnabled NOTIFY monitorChanged)

public:
    Metronome(AudioProcessor *processor, QObject *parent = nullptr);
    ~Metronome();

    bool monitorEnabled() const;

signals:
    void beatChanged(int beat);
    void bpmChanged(int bpm);
    void bpiChanged(int bpi);
    void monitorChanged(bool monitor);

public slots:
    void start();
    void stop();
    void processAudioStreams();

    // Set the BPM and BPI values for the next interval
    void setNextBpmBpi(int bpm, int bpi);

    // Mute/unmute audio
    void setMonitorEnabled(bool monitor);

private:
    QTimer nextBeatTimer;
    AudioProcessor *processor;
    AudioStream *stream;
    std::vector<float> click;
    int beat_;
    int bpm_;
    int bpi_;
    int nextBpm; // takes effect next interval
    int nextBpi;
    size_t samplesPerBeat;
    SampleTime nextBeatSampleTime; // for syncing QTimer to audio stream
    SampleTime startTime;
    SampleTime now;
    SampleTime intervalPos; // number of samples from start of interval
    SampleTime writeSampleTime; // stream write position
    bool monitor;

private slots:
    void nextBeat();
};
