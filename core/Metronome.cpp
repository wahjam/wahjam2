// SPDX-License-Identifier: Apache-2.0
#include <assert.h>
#include <QFile>

#include "Metronome.h"

Metronome::Metronome(AppView *appView_, QObject *parent)
    : QObject{parent}, appView{appView_}, stream{nullptr},
      beat_{1}, bpm_{120}, bpi_{16}, nextBpm{120}, nextBpi{16},
      nextBeatSampleTime{0}, writeIntervalPos{0}, writeSampleTime{0},
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

void Metronome::nextBeat()
{
    bool emitBpmChanged = false;
    bool emitBpiChanged = false;

    beat_++;
    if (beat_ > bpi_) {
        beat_ = 1;
        emitBpmChanged = bpm_ != nextBpm;
        emitBpiChanged = bpi_ != nextBpi;
        bpm_ = nextBpm;
        bpi_ = nextBpi;
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
    int sampleRate = appView->audioProcessor()->getSampleRate();
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

    // Kick off counting using nextBeat()
    beat_ = nextBpi;
    bpm_ = nextBpm;
    bpi_ = nextBpi;
    nextBeatSampleTime = appView->currentSampleTime();
    nextBeat();

    appView->audioProcessor()->addPlaybackStream(stream);
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
        writeSampleTime = 0;
        writeIntervalPos = 0;
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
