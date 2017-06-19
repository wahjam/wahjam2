#include <string.h>
#include <algorithm>
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
    ring.setSize((nsamples + SMALL_BLOCK_SIZE - 1) / SMALL_BLOCK_SIZE);
    delete [] sampleBuffer;
    sampleBuffer = new float[nsamples];
    sampleBufferSize = nsamples;
    writeIndex = 0;
    samplesQueued.store(0);
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

    // Writes may cross the end of the sample buffer...
    while (nsamples > 0) {
        if (!ring.canWrite()) {
            return nwritten;
        }

        size_t n = std::min(sampleBufferSize - writeIndex, nsamples);
        if (samplesQueued.load() + n > sampleBufferSize) {
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

size_t AudioStream::readInternal(SampleTime now, float *samples,
                                 size_t nsamples, bool mix, float mixVol)
{
    size_t nread = 0;

    // Reads may seek ahead or cross the end of the sample buffer...
    while (nsamples > 0) {
        while (!ring.canRead()) {
            return nread;
        }

        AudioDescriptor &desc = ring.readCurrent();

        // Seek if necessary
        size_t seek = now - desc.time;
        if (seek > desc.nsamples) {
            size_t dequeued = desc.nsamples;
            ring.readNext();
            samplesQueued.fetch_sub(dequeued);
            continue;
        }

        size_t n = std::min(desc.nsamples - seek, nsamples);
        if (mix) {
            mixSamples(&desc.samples[seek], samples, n, mixVol);
        } else {
            memcpy(samples, &desc.samples[seek], n * sizeof(float));
        }

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
        samples += n;
        nsamples -= n;
        nread += n;
    }

    return nread;
}

size_t AudioStream::read(SampleTime now, float *samples, size_t nsamples)
{
    return readInternal(now, samples, nsamples, false, 0.f);
}

size_t AudioStream::readMix(SampleTime now, float *samples,
                            size_t nsamples, float mixVol)
{
    return readInternal(now, samples, nsamples, true, mixVol);
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
