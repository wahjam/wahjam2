// SPDX-License-Identifier: Apache-2.0
#include <assert.h>
#include <QFile>

#include "Metronome.h"

Metronome::Metronome(AudioProcessor *processor_, QObject *parent)
    : QObject{parent}, processor{processor_}, bpm_{120},
      samplesPerBeat{0}, startTime{0}, now{0}
{
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

void Metronome::start()
{
    if (stream) {
        return;
    }

    stream = new AudioStream;

    qDebug("%s stream %p", __func__, stream);

    processor->addPlaybackStream(stream);
}

void Metronome::stop()
{
    qDebug("%s stream %p", __func__, stream);

    if (stream) {
        processor->removePlaybackStream(stream);
        stream = nullptr;
    }
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
