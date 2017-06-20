#include <assert.h>
#include <QResource>

#include "Metronome.h"

Metronome::Metronome(AudioProcessor *processor_, QObject *parent)
    : QObject{parent}, processor{processor_}, stream{nullptr}, bpm_{120},
      samplesPerBeat{0}, startTime{0}, now{0}
{
    // Load resource
    QResource resource{"/click.raw"};
    assert(resource.isValid());
    if (resource.isCompressed()) {
        QByteArray uncompressed = qUncompress(QByteArray{reinterpret_cast<const char*>(resource.data())});
        clickLen = uncompressed.size();
        click = new float[clickLen];
        memcpy(click, uncompressed.constData(), clickLen);
    } else {
        clickLen = resource.size();
        click = new float[clickLen];
        memcpy(click, resource.data(), clickLen);
    }
}

Metronome::~Metronome()
{
    stop();
    delete [] click;
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
    float *buf = new float[nsamples];
    size_t offset = (now - startTime) % samplesPerBeat;

    for (size_t i = 0; i < nsamples; i++) {
        buf[i] = offset < clickLen ? click[offset] : 0.f;
        offset = (offset + 1) % samplesPerBeat;
    }

    stream->write(now, buf, nsamples);
    now += nsamples;

    delete [] buf;
}
