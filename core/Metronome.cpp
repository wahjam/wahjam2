// SPDX-License-Identifier: Apache-2.0
#include <assert.h>
#include <QFile>

#include "Metronome.h"

Metronome::Metronome(AppView *appView_, QObject *parent)
    : QObject{parent}, appView{appView_}, stream{nullptr},
      beat_{1}, bpm_{120}, bpi_{16}, nextBpm{120}, nextBpi{16},
      nextIntervalTime_{0}, nextBeatSampleTime{0},
      writeIntervalPos{0}, writeSampleTime{0},
      monitor{true}
{
    nextBeatTimer.setSingleShot(true);
    connect(&nextBeatTimer, &QTimer::timeout,
            this, &Metronome::nextBeat);

    QFile file{":/click.raw"};
    if (!file.open(QIODevice::ReadOnly)) {
        qCritical("Cannot open click.raw resource");
    }

    QByteArray data = file.readAll();
    auto samples = reinterpret_cast<const float*>(data.constData());
    auto nsamples = data.size() / sizeof(float);

    click = std::vector<float>(nsamples);
    for (size_t i = 0; i < nsamples; i++) {
        click[i] = samples[i];
    }
}

Metronome::~Metronome()
{
    stop();
}

SampleTime Metronome::nextIntervalTime() const
{
    return nextIntervalTime_;
}

size_t Metronome::remainingIntervalTime(SampleTime pos) const
{
    SampleTime intervalTime;
    SampleTime intervalDuration;
    int sampleRate = appView->audioProcessor()->getSampleRate();

    if (pos < nextIntervalTime_) {
        SampleTime samplesPerBeat = 60.f / bpm_ * sampleRate;
        intervalDuration = bpi_ * samplesPerBeat;
        intervalTime = nextIntervalTime_ - intervalDuration;
    } else {
        SampleTime samplesPerBeat = 60.f / nextBpm * sampleRate;
        intervalDuration = nextBpi * samplesPerBeat;
        intervalTime = nextIntervalTime_;
    }

    // Check that pos is within the interval that we've calculated. If not,
    // then either the caller needs to avoid getting out of sync or this method
    // needs to be extended to keep more bpi/bpm history.
    assert(pos >= intervalTime);
    assert(pos < intervalTime + intervalDuration);

    return intervalTime + intervalDuration - pos;
}

void Metronome::nextBeat()
{
    int sampleRate = appView->audioProcessor()->getSampleRate();
    bool emitBpmChanged = false;
    bool emitBpiChanged = false;

    beat_++;
    if (beat_ > bpi_) {
        beat_ = 1;
        emitBpmChanged = bpm_ != nextBpm;
        emitBpiChanged = bpi_ != nextBpi;
        bpm_ = nextBpm;
        bpi_ = nextBpi;
        nextIntervalTime_ += bpi_ * 60.f / bpm_ * sampleRate;
    }

    if (emitBpmChanged) {
        emit bpmChanged(bpm_);
    }
    if (emitBpiChanged) {
        emit bpiChanged(bpi_);
    }
    qDebug("beatChanged %d/%d", beat_, bpi_);
    emit beatChanged(beat_);

    // QTimer only has millisecond accuracy so sync against sample time to
    // avoid accumulating errors.
    SampleTime samplesPerBeat = 60.f / bpm_ * sampleRate;
    auto t = appView->currentSampleTime();
    auto samples = nextBeatSampleTime + samplesPerBeat - t;
    auto msec = samplesToMsec(sampleRate, samples);
    nextBeatTimer.start(msec);
    nextBeatSampleTime += samplesPerBeat;
}

void Metronome::setNextBpmBpi(int bpm, int bpi)
{
    nextBpm = bpm;
    nextBpi = bpi;
}

void Metronome::start()
{
    if (stream) {
        return;
    }

    stream = new AudioStream;
    stream->setMonitorEnabled(monitor);

    writeIntervalPos = 0;

    // Kick off counting using nextBeat()
    beat_ = nextBpi;
    bpm_ = nextBpm;
    bpi_ = nextBpi;
    nextIntervalTime_ = appView->currentSampleTime();
    nextBeatSampleTime = nextIntervalTime_;
    nextBeat();

    appView->audioProcessor()->addPlaybackStream(stream);
    processAudioStreams();
}

void Metronome::stop()
{
    if (stream) {
        appView->audioProcessor()->removePlaybackStream(stream);
        stream = nullptr;
    }

    nextBeatTimer.stop();
}

void Metronome::processAudioStreams()
{
    if (!stream) {
        return;
    }

    if (stream->checkResetAndClear()) {
        writeSampleTime = appView->currentSampleTime();
    }

    /*
     * Audio samples are pre-rendered in the Qt thread instead of rendered in
     * the real-time audio thread. The BPM/BPI could be changed after samples
     * for the next interval have already been rendered. Ignore this for now
     * because it should not be very noticable. nextBeat() will update bpm_ and
     * bpi_ eventually so further audio samples will be rendered correctly.
     */
    int sampleRate = appView->audioProcessor()->getSampleRate();
    SampleTime samplesPerBeat = 60.f / bpm_ * sampleRate;
    SampleTime samplesPerInterval = bpi_ * samplesPerBeat;
    size_t nsamples = stream->numSamplesWritable();
    std::vector<float> buf(nsamples);
    size_t offset = writeIntervalPos % samplesPerBeat;

    for (size_t i = 0; i < nsamples; i++) {
        buf[i] = offset < click.size() ? click[offset] : 0.f;
        offset = (offset + 1) % samplesPerBeat;
        writeIntervalPos = (writeIntervalPos + 1) % samplesPerInterval;
    }

    stream->write(writeSampleTime, buf.data(), nsamples);
    writeSampleTime += nsamples;

    // Periodically emit signal since peak volume is always changing
    emit peakVolumeChanged(stream->getPeakVolume());
}

bool Metronome::monitorEnabled() const
{
    return monitor;
}

void Metronome::setMonitorEnabled(bool monitor_)
{
    if (monitor_ == monitor) {
        return; // unchanged
    }

    monitor = monitor_;
    emit monitorChanged(monitor);
    if (stream) {
        stream->setMonitorEnabled(monitor);
    }
}

float Metronome::peakVolume() const
{
    if (!stream) {
        return 0.f;
    }
    return stream->getPeakVolume();
}
