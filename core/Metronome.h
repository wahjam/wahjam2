// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <QTimer>

#include "AppView.h"

class Metronome : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int beat MEMBER beat_ NOTIFY beatChanged)
    Q_PROPERTY(int bpm MEMBER bpm_ NOTIFY bpmChanged)
    Q_PROPERTY(int bpi MEMBER bpi_ NOTIFY bpiChanged)
    Q_PROPERTY(bool monitor READ monitorEnabled WRITE setMonitorEnabled NOTIFY monitorChanged)
    Q_PROPERTY(float peakVolume READ peakVolume NOTIFY peakVolumeChanged)
    Q_PROPERTY(float gain READ gain WRITE setGain NOTIFY gainChanged)
    Q_PROPERTY(QString accentFilename READ accentFilename WRITE setAccentFilename NOTIFY accentFilenameChanged)
    Q_PROPERTY(QString clickFilename READ clickFilename WRITE setClickFilename NOTIFY clickFilenameChanged)

public:
    Metronome(AppView *appView, QObject *parent = nullptr);
    ~Metronome();

    bool monitorEnabled() const;
    float peakVolume() const;
    float gain() const;
    QString accentFilename() const;
    QString clickFilename() const;

    void setGain(float value);
    void setAccentFilename(const QString &filename);
    void setClickFilename(const QString &filename);

    SampleTime currentIntervalTime() const;
    SampleTime nextIntervalTime() const;
    size_t remainingIntervalTime(SampleTime pos) const;

signals:
    void beatChanged(int beat);
    void bpmChanged(int bpm);
    void bpiChanged(int bpi);
    void monitorChanged(bool monitor);
    void peakVolumeChanged();
    void gainChanged();
    void accentFilenameChanged(const QString &filename);
    void clickFilenameChanged(const QString &filename);

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
    AppView *appView;
    AudioStream *stream;
    QString accentFilename_;
    QString clickFilename_;
    std::vector<float> accent;
    std::vector<float> click;
    int beat_;
    int bpm_;
    int bpi_;
    int nextBpm; // takes effect next interval
    int nextBpi;
    SampleTime currentIntervalTime_; // first sample of the next interval
    SampleTime nextIntervalTime_; // first sample of the next interval
    SampleTime nextBeatSampleTime; // for syncing QTimer to audio stream
    SampleTime writeIntervalPos; // number of samples from start of interval
    SampleTime writeSampleTime; // stream write position
    bool monitor;

    void loadSamples();
    void nextBeat();

private slots:
    void checkNextBeat();
};
