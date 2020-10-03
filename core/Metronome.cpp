// SPDX-License-Identifier: Apache-2.0
#include <assert.h>
#include <QFile>

#include "Metronome.h"

Metronome::Metronome(AudioProcessor *processor_, QObject *parent)
    : QObject{parent}, processor{processor_}, stream{nullptr},
      bpm_{120}, samplesPerBeat{0}, startTime{0}, now{0},
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
    if (beat_ == bpi_) {
        beat_ = 0;
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
    emit beatChanged(beat_);

    // QTimer only has millisecond accuracy so sync against sample time to
    // avoid accumulating errors.
    SampleTime samplesPerBeat = 60.f / bpm_ * processor->getSampleRate();
    auto t = processor->getNextSampleTime(); // TODO introduce a real time-synced sample time
    auto samples = nextBeatSampleTime + samplesPerBeat - t;
    auto msec = samples * 1000 / processor->getSampleRate();
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

    qDebug("%s stream %p", __func__, stream);

    processor->addPlaybackStream(stream);

    beat_ = 0; // TODO should beat counting be 0-based or 1-based?
    nextBpm = bpm_;
    nextBpi = bpi_;
    nextBeatSampleTime = processor->getNextSampleTime(); // TODO
    nextBeat();
}

void Metronome::stop()
{
    qDebug("%s stream %p", __func__, stream);

    if (stream) {
        processor->removePlaybackStream(stream);
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
        qDebug("%s audio stream was reset", __func__);
        // TODO implement bpm change
        samplesPerBeat = 60.f / bpm_ * processor->getSampleRate();
        startTime = processor->getNextSampleTime();
        now = startTime;
    }

    size_t nsamples = stream->numSamplesWritable();
    std::vector<float> buf(nsamples);
    size_t offset = (now - startTime) % samplesPerBeat;

    for (size_t i = 0; i < nsamples; i++) {
        buf[i] = offset < click.size() ? click[offset] : 0.f;
        offset = (offset + 1) % samplesPerBeat;
    }

    stream->write(now, buf.data(), nsamples);
    now += nsamples;
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
