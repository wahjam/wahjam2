#include <string.h>
#include <algorithm>
#include <QtGlobal>
#include "AudioStream.h"

enum
{
    SMALL_BLOCK_SIZE = 64 /* samples */,
};

AudioStream::AudioStream(size_t sampleBufferSize_)
    : sampleBuffer{nullptr}, pan{0.f}, monitor{true}
{
    setSampleBufferSize(sampleBufferSize_);
}

AudioStream::~AudioStream()
{
    delete [] sampleBuffer;
}

void AudioStream::setSampleBufferSize(size_t nsamples)
{
    wasReset = true;
    ring.setSize((nsamples + SMALL_BLOCK_SIZE - 1) / SMALL_BLOCK_SIZE);
    delete [] sampleBuffer;
    sampleBuffer = new float[nsamples];
    sampleBufferSize = nsamples;
    writeIndex = 0;
    samplesQueued.store(0);
}

bool AudioStream::checkResetAndClear()
{
    bool reset = wasReset;
    wasReset = false;
    return reset;
}

size_t AudioStream::numSamplesWritable() const
{
    return sampleBufferSize - samplesQueued.load();
}

size_t AudioStream::numSamplesReadable() const
{
    return samplesQueued.load();
}

size_t AudioStream::write(SampleTime now, const float *samples, size_t nsamples)
{
    size_t nwritten = 0;

//    qDebug("%s stream %p now %llu nsamples %zu", __func__, this, now, nsamples);

    // Writes may cross the end of the sample buffer...
    while (nsamples > 0) {
        if (!ring.canWrite()) {
//            qDebug("%s stream %p ring full", __func__, this);
            return nwritten;
        }

        size_t n = std::min(sampleBufferSize - writeIndex, nsamples);
        if (samplesQueued.load() + n > sampleBufferSize) {
//            qDebug("%s stream %p buffer space exhausted", __func__, this);
            return nwritten;
        }

        AudioDescriptor desc{
            .samples = &sampleBuffer[writeIndex],
            .nsamples = n,
            .time = now,
        };

        memcpy(desc.samples, samples, n * sizeof(float));

        samplesQueued.fetch_add(n);

        writeIndex = (writeIndex + n) % sampleBufferSize;
        now += n;
        samples += n;
        nsamples -= n;

        ring.writeCurrent() = desc;
        ring.writeNext();

        nwritten += n;
    }
    return nwritten;
}

size_t AudioStream::readInternal(SampleTime now,
                                 std::function<ReadFn> fn,
                                 size_t nsamples)
{
    size_t nread = 0;

    // Reads may seek ahead or cross the end of the sample buffer...
    while (nsamples > 0) {
        while (!ring.canRead()) {
//            qDebug("%s stream %p reached end of ring", __func__, this);
            return nread;
        }

        AudioDescriptor &desc = ring.readCurrent();

        // Seek if necessary
        size_t seek = now - desc.time;
        if (seek > desc.nsamples) {
            size_t dequeued = desc.nsamples;
            ring.readNext();
            samplesQueued.fetch_sub(dequeued);
//            qDebug("%s stream %p seeking to time %llu, dropping %zu samples", __func__, this, now, dequeued);
            continue;
        }

        size_t n = std::min(desc.nsamples - seek, nsamples);
//        qDebug("%s stream %p processing %zu samples", __func__, this, n);
        fn(nread, &desc.samples[seek], n);

        // Complete a descriptor
        size_t end = seek + n;
        if (end == desc.nsamples) {
            ring.readNext();
        } else {
            desc.nsamples -= end;
            desc.samples += end;
            desc.time += end;
        }

        samplesQueued.fetch_sub(end);

        now += n;
        nsamples -= n;
        nread += n;
    }

//    qDebug("%s stream %p nread %zu", __func__, this, nread);
    return nread;
}

size_t AudioStream::readDiscard(SampleTime now, size_t nsamples)
{
    return readInternal(now, [](size_t, const float*, size_t) {}, nsamples);
}

size_t AudioStream::read(SampleTime now, float *samples, size_t nsamples)
{
    return readInternal(now,
        [samples](size_t offset, const float *input, size_t n) {
            memcpy(&samples[offset], input, n * sizeof(float));
        },
        nsamples);
}

size_t AudioStream::readMixStereo(SampleTime now,
                                  float *samples[CHANNELS_STEREO],
                                  size_t nsamples)
{
    // TODO use -4.5 dB pan law instead of linear panning?
    const float pan = getPan();
    const float volLeft = (1.f - pan) / 2;
    const float volRight = (pan + 1.f) / 2;

    return readInternal(now,
        [samples, volLeft, volRight](size_t offset, const float *input, size_t n) {
            mixSamples(input, &samples[CHANNEL_LEFT][offset], n, volLeft);
            mixSamples(input, &samples[CHANNEL_RIGHT][offset], n, volRight);
        },
        nsamples);
}

float AudioStream::getPan() const
{
    return pan.load();
}

void AudioStream::setPan(float pan_)
{
    pan.store(pan_);
}

bool AudioStream::monitorEnabled() const
{
    return monitor.load();
}

void AudioStream::setMonitorEnabled(bool enabled)
{
    monitor.store(enabled);
}
