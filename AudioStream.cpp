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

bool AudioStream::write(SampleTime now, const float *samples, size_t nsamples)
{
    // Writes may cross the end of the sample buffer...
    while (nsamples > 0) {
        if (!ring.canWrite()) {
            return false;
        }

        size_t n = std::min(sampleBufferSize - writeIndex, nsamples);
        if (samplesQueued.load() + n > sampleBufferSize) {
            return false;
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
    }
    return true;
}

bool AudioStream::readInternal(SampleTime now, float *samples,
                               size_t nsamples, bool mix, float mixVol)
{
    // Reads may seek ahead or cross the end of the sample buffer...
    while (nsamples > 0) {
        while (!ring.canRead()) {
            return false;
        }

        const AudioDescriptor &desc = ring.readCurrent();

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
            samplesQueued.fetch_sub(end);
        }

        now += n;
        samples += n;
        nsamples -= n;
    }

    return true;
}

bool AudioStream::read(SampleTime now, float *samples, size_t nsamples)
{
    return readInternal(now, samples, nsamples, false, 0.f);
}

bool AudioStream::readMix(SampleTime now, float *samples,
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
