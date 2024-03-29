// SPDX-License-Identifier: Apache-2.0
#include <assert.h>
#include <QFile>

#include "OggVorbisDecoder.h"
#include "Resampler.h"
#include "Metronome.h"

Metronome::Metronome(AppView *appView_, QObject *parent)
    : QObject{parent}, appView{appView_}, stream{nullptr},
      accentFilename_{":/data/accent.ogg"},
      clickFilename_{":/data/click.ogg"},
      beat_{1}, bpm_{120}, bpi_{16}, nextBpm{120}, nextBpi{16},
      currentIntervalTime_{0},
      nextIntervalTime_{0}, nextBeatSampleTime{0},
      writeIntervalPos{0}, writeSampleTime{0},
      monitor{true}
{
    nextBeatTimer.setSingleShot(true);
    nextBeatTimer.setTimerType(Qt::PreciseTimer);
    connect(&nextBeatTimer, &QTimer::timeout,
            this, &Metronome::checkNextBeat);
}

Metronome::~Metronome()
{
    stop();
}

QString Metronome::accentFilename() const
{
    return accentFilename_;
}

QString Metronome::clickFilename() const
{
    return clickFilename_;
}

void Metronome::setAccentFilename(const QString &filename)
{
    accentFilename_ = filename;
    emit accentFilenameChanged(filename);
}

void Metronome::setClickFilename(const QString &filename)
{
    clickFilename_ = filename;
    emit clickFilenameChanged(filename);
}

SampleTime Metronome::currentIntervalTime() const
{
    return currentIntervalTime_;
}

SampleTime Metronome::nextIntervalTime() const
{
    return nextIntervalTime_;
}

size_t Metronome::remainingIntervalTime(SampleTime pos) const
{
    // We don't keep a history of past intervals
    assert(pos >= currentIntervalTime_);

    if (pos < nextIntervalTime_) {
        return nextIntervalTime_ - pos;
    } else {
        int sampleRate = appView->audioProcessor()->getSampleRate();
        SampleTime end = nextIntervalTime_ + (nextBpi * 60. / nextBpm * sampleRate);

        // We don't know future interval bpi/bpm
        assert(pos <= end);

        return end - pos;
    }
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
        currentIntervalTime_ = nextIntervalTime_;
        nextIntervalTime_ += bpi_ * 60. / bpm_ * sampleRate;
    }

    nextBeatSampleTime += 60. / bpm_ * sampleRate;

    if (emitBpmChanged) {
        emit bpmChanged(bpm_);
    }
    if (emitBpiChanged) {
        emit bpiChanged(bpi_);
    }
    qDebug("beatChanged %d/%d", beat_, bpi_);
    emit beatChanged(beat_);
}

void Metronome::checkNextBeat()
{
    while (nextBeatSampleTime <= appView->currentSampleTime()) {
        nextBeat();
    }

    // QTimer only has millisecond accuracy so sync against sample time to
    // avoid accumulating errors.
    int sampleRate = appView->audioProcessor()->getSampleRate();
    auto t = appView->currentSampleTime();
    auto samples = t > nextBeatSampleTime ? 0 : nextBeatSampleTime - t;
    auto msec = samplesToMsec(sampleRate, samples);
    nextBeatTimer.start(msec);
}

void Metronome::setNextBpmBpi(int bpm, int bpi)
{
    nextBpm = bpm;
    nextBpi = bpi;
}

static std::vector<float> loadOggSamples(const QString &filename, int outputSampleRate)
{
    int inputSampleRate;
    QByteArray left;
    QByteArray right;
    size_t nsamples = OggVorbisDecoder::decodeFile(filename.toUtf8().constData(),
                                                   &left, &right,
                                                   &inputSampleRate);
    if (nsamples == 0) {
        return {};
    }

    double ratio = static_cast<double>(outputSampleRate) / inputSampleRate;
    Resampler resampler;
    resampler.setRatio(ratio);
    resampler.appendData(left);
    resampler.finishAppendingData();

    QByteArray output;
    while (resampler.resample(&output, 8192) > 0) {
        // Do nothing
    }

    // The right channel is ignored
    nsamples = output.size() / sizeof(float);
    std::vector<float> samples(nsamples);
    const float *data = reinterpret_cast<const float*>(output.constData());
    for (size_t i = 0; i < nsamples; i++) {
        samples[i] = data[i];
    }
    return samples;
}

void Metronome::loadSamples()
{
    int sampleRate = appView->audioProcessor()->getSampleRate();

    click = loadOggSamples(clickFilename_, sampleRate);

    if (accentFilename_.isEmpty()) {
        accent = click;
    } else {
        accent = loadOggSamples(accentFilename_, sampleRate);
    }
}

void Metronome::start()
{
    if (stream) {
        return;
    }

    stream = new AudioStream;
    stream->setMonitorEnabled(monitor);

    writeIntervalPos = 0;

    // Kick off counting using checkNextBeat()
    beat_ = nextBpi;
    bpm_ = nextBpm;
    bpi_ = nextBpi;
    nextIntervalTime_ = appView->currentSampleTime();
    nextBeatSampleTime = nextIntervalTime_;
    checkNextBeat();

    appView->audioProcessor()->addPlaybackStream(stream);
    processAudioStreams();
    emit gainChanged();
}

void Metronome::stop()
{
    if (stream) {
        appView->audioProcessor()->removePlaybackStream(stream);
        stream = nullptr;
        emit peakVolumeChanged();
        emit gainChanged();
    }

    nextBeatTimer.stop();
}

void Metronome::processAudioStreams()
{
    if (!stream) {
        return;
    }

    if (stream->checkResetAndClear()) {
        loadSamples();
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
    SampleTime samplesPerInterval = bpi_ * 60. / bpm_ * sampleRate;
    SampleTime samplesPerBeat = 60. / bpm_ * sampleRate;
    size_t nsamples = stream->numSamplesWritable();
    std::vector<float> buf(nsamples);
    size_t offset = writeIntervalPos % samplesPerBeat;

    for (size_t i = 0; i < nsamples; i++) {
        const std::vector<float> &clip =
            writeIntervalPos / samplesPerBeat == 0 ?
            accent : click;

        buf[i] = offset < clip.size() ? clip[offset] : 0.f;
        offset = (offset + 1) % samplesPerBeat;
        writeIntervalPos = (writeIntervalPos + 1) % samplesPerInterval;
    }

    stream->write(writeSampleTime, buf.data(), nsamples);
    writeSampleTime += nsamples;

    // Periodically emit signal since peak volume is always changing
    emit peakVolumeChanged();
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

float Metronome::gain() const
{
    if (!stream) {
        return 0.f;
    }
    return stream->getGain();
}

void Metronome::setGain(float gain)
{
    if (stream) {
        qDebug("setGain %g", (double)gain);
        stream->setGain(gain);
        emit gainChanged();
    }
}
