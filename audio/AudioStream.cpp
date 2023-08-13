// SPDX-License-Identifier: Apache-2.0
#include <string.h>
#include <algorithm>
#include "AudioStream.h"

AudioStream::AudioStream(AudioStream::Type type_, size_t sampleBufferSize_)
    : type{type_}, sampleBuffer{nullptr}, gain{1.f}, peakVolume{0.f},
      peakVolumeDecay{0.f}, pan{0.f}, monitor{true}
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
    ring.setSize(nsamples);
    delete [] sampleBuffer;
    sampleBuffer = new float[nsamples];
    sampleBufferSize = nsamples;
    writeIndex = 0;
    samplesQueued.store(0);
}

void AudioStream::setPeakVolumeDecay(float decay)
{
    peakVolumeDecay = decay;
}

bool AudioStream::checkResetAndClear()
{
    bool reset = wasReset;
    wasReset = false;
    return reset;
}

bool AudioStream::peekReadSampleTime(SampleTime *sampleTime) const
{
    if (!ring.canRead()) {
        return false;
    }

    auto desc = ring.readCurrent();
    *sampleTime = desc.time;
    return true;
}

size_t AudioStream::numSamplesWritable() const
{
    return sampleBufferSize - samplesQueued.load();
}

size_t AudioStream::numSamplesReadable() const
{
    return samplesQueued.load();
}

void AudioStream::updatePeakVolume(const float *samples, size_t nsamples)
{
    float peak = peakVolume.load();
    peak = ::updatePeakVolume(samples, nsamples, peakVolumeDecay, peak);
    peakVolume.store(peak);
}

float AudioStream::getPeakVolume() const
{
    return peakVolume.load();
}

size_t AudioStream::write(SampleTime now, const float *samples, size_t nsamples)
{
    size_t nwritten = 0;

    if (type == CAPTURE) {
        updatePeakVolume(samples, nsamples);
    }

    // Writes may cross the end of the sample buffer...
    while (nsamples > 0) {
        if (!ring.canWrite()) {
            return nwritten;
        }

        size_t n = std::min(sampleBufferSize - writeIndex, nsamples);
        if (samplesQueued.load() + n > sampleBufferSize) {
            return nwritten;
        }

        AudioDescriptor desc{&sampleBuffer[writeIndex], n, now};

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
            return nread;
        }

        AudioDescriptor &desc = ring.readCurrent();

        // Stop if the audio data is in the future. Don't bother handling
        // partial overlap, we'll drop the overlapping audio samples and seek
        // into the descriptor next time.
        if (now < desc.time) {
            return nread;
        }

        // Seek if necessary
        size_t seek = now - desc.time;
        if (seek >= desc.nsamples) {
            size_t dequeued = desc.nsamples;
            ring.readNext();
            samplesQueued.fetch_sub(dequeued);
            continue;
        }

        size_t n = std::min(desc.nsamples - seek, nsamples);

        if (type == PLAYBACK) {
            updatePeakVolume(&desc.samples[seek], n);
        }

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

    return nread;
}

size_t AudioStream::readDiscard(SampleTime now, size_t nsamples)
{
    return readInternal(now, [](size_t, const float*, size_t) {}, nsamples);
}

size_t AudioStream::read(SampleTime now, float *samples, size_t nsamples)
{
    const float vol = getGain();

    return readInternal(now,
        [samples, vol](size_t offset, const float *input, size_t n) {
            applyGain(input, &samples[offset], n, vol);
        },
        nsamples);
}

size_t AudioStream::readMixStereo(SampleTime now,
                                  float *samples[CHANNELS_STEREO],
                                  size_t nsamples)
{
    // TODO use -4.5 dB pan law instead of linear panning?
    const float pan = getPan();
    const float vol = getGain();
    const float volLeft = vol * (1.f - pan) / 2;
    const float volRight = vol * (pan + 1.f) / 2;

    return readInternal(now,
        [samples, volLeft, volRight](size_t offset, const float *input, size_t n) {
            mixSamples(input, &samples[CHANNEL_LEFT][offset], n, volLeft);
            mixSamples(input, &samples[CHANNEL_RIGHT][offset], n, volRight);
        },
        nsamples);
}

void AudioStream::readDiscardAll()
{
    while (ring.canRead()) {
        AudioDescriptor &desc = ring.readCurrent();
        size_t dequeued = desc.nsamples;
        ring.readNext();
        samplesQueued.fetch_sub(dequeued);
    }
}

float AudioStream::getGain() const
{
    return gain.load();
}

void AudioStream::setGain(float gain_)
{
    gain.store(gain_);
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
