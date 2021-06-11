// SPDX-License-Identifier: Apache-2.0
#include <assert.h>
#include <string.h>
#include "AudioProcessor.h"

AudioProcessor::AudioProcessor()
    : rcu{},
      playbackStreams{&rcu, new PlaybackStreams},
      captureStreams{
          {AudioStream::CAPTURE, false},
          {AudioStream::CAPTURE, false}
      },
      sampleRate{44100},
      running{false},
      nextSampleTime{0},
      masterGain{1.f},
      masterPeakVolume{{0.f}, {0.f}},
      peakVolumeDecay{1.f}
{
}

AudioProcessor::~AudioProcessor()
{
    // No streams should be left. Their owner has a pointer so we cannot delete
    // them here.
    assert(playbackStreams.load()->empty());

    // Delete the playback streams vector itself
    playbackStreams.store(nullptr);
}

void AudioProcessor::addPlaybackStream(AudioStream *stream)
{
    size_t nsamples = running.load() ?
        msecToSamples(getSampleRate(), GENEROUS_BUFFER_MSEC) : 0;
    stream->setSampleBufferSize(nsamples);
    stream->setPeakVolumeDecay(peakVolumeDecay);

    auto newStreams = new PlaybackStreams{*playbackStreams.load()};
    newStreams->push_back(RCUPointer<AudioStream>{&rcu, stream});
    playbackStreams.store(newStreams);
}

void AudioProcessor::removePlaybackStream(AudioStream *stream)
{
    auto newStreams = new PlaybackStreams{*playbackStreams.load()};

    for (auto i = newStreams->begin(); i != newStreams->end(); ++i) {
        if ((*i).load() == stream) {
            (*i).store(nullptr);    // RCU delete the stream itself
            newStreams->erase(i);
            break;
        }
    }

    playbackStreams.store(newStreams);
}

AudioStream &AudioProcessor::captureStream(int channel)
{
    return captureStreams[channel];
}

void AudioProcessor::tick()
{
    rcu.reclaim();
}

SampleTime AudioProcessor::getNextSampleTime() const
{
    return nextSampleTime.load();
}

void AudioProcessor::processInputs(float *inOutSamples[CHANNELS_STEREO], size_t nsamples, SampleTime now)
{
    for (int ch = 0; ch < CHANNELS_STEREO; ch++) {
        AudioStream &input = captureStream(ch);

        input.write(now, inOutSamples[ch], nsamples);

        // Apply gain to monitor signal
        if (input.monitorEnabled()) {
            applyGain(inOutSamples[ch], inOutSamples[ch], nsamples, input.getGain());
        } else {
            memset(inOutSamples[ch], 0, nsamples * sizeof(float));
        }
    }
}

void AudioProcessor::mixPlaybackStreams(float *inOutSamples[CHANNELS_STEREO], size_t nsamples, SampleTime now)
{
    PlaybackStreams *streams = playbackStreams.load();

    for (auto streamPointer : *streams) {
        AudioStream *stream = streamPointer.load();

        if (!stream->monitorEnabled()) {
            stream->readDiscard(now, nsamples);
            continue;
        }

        stream->readMixStereo(now, inOutSamples, nsamples);
    }
}

void AudioProcessor::process(float *inOutSamples[CHANNELS_STEREO],
                             size_t nsamples,
                             SampleTime now)
{
    RCUReadLocker readLocker{&rcu};

    nextSampleTime.store(now + nsamples);

    if (!isRunning()) {
        for (int ch = 0; ch < CHANNELS_STEREO; ch++) {
            memset(inOutSamples[ch], 0, nsamples * sizeof(float));
        }
        return;
    }

    processInputs(inOutSamples, nsamples, now);
    mixPlaybackStreams(inOutSamples, nsamples, now);

    const float gain = getMasterGain();
    for (int ch = 0; ch < CHANNELS_STEREO; ch++) {
        applyGain(inOutSamples[ch], inOutSamples[ch], nsamples, gain);

        float peak = getMasterPeakVolume(ch);
        peak = updatePeakVolume(inOutSamples[ch], nsamples,
                                peakVolumeDecay, peak);
        masterPeakVolume[ch].store(peak);
    }
}

int AudioProcessor::getSampleRate() const
{
    return sampleRate.load();
}

void AudioProcessor::setSampleRate(int rate)
{
    sampleRate.store(rate);

    peakVolumeDecay = powf(0.01, 1 / (float)rate); // -40 dB/sec decay
}

float AudioProcessor::getMasterGain() const
{
    return masterGain.load();
}

void AudioProcessor::setMasterGain(float gain)
{
    masterGain.store(gain);
}

float AudioProcessor::getMasterPeakVolume(int channel) const
{
    return masterPeakVolume[channel].load();
}

void AudioProcessor::setSampleBufferSize(size_t nsamples)
{
    for (int ch = 0; ch < CHANNELS_STEREO; ch++) {
        captureStreams[ch].setSampleBufferSize(nsamples);
    }

    for (auto streamPointer : *playbackStreams.load()) {
        AudioStream *stream = streamPointer.load();

        stream->setSampleBufferSize(nsamples);
    }
}

void AudioProcessor::setPeakVolumeDecay()
{
    for (int ch = 0; ch < CHANNELS_STEREO; ch++) {
        captureStreams[ch].setPeakVolumeDecay(peakVolumeDecay);
    }

    for (auto streamPointer : *playbackStreams.load()) {
        AudioStream *stream = streamPointer.load();

        stream->setPeakVolumeDecay(peakVolumeDecay);
    }
}

void AudioProcessor::setRunning(bool enabled)
{
    if (running.load() == enabled) {
        return;
    }

    if (enabled) {
        size_t nsamples = msecToSamples(getSampleRate(), GENEROUS_BUFFER_MSEC);
        setSampleBufferSize(nsamples);
        setPeakVolumeDecay();
    }

    running.store(enabled);

    if (!enabled) {
        setSampleBufferSize(0);

        masterPeakVolume[CHANNEL_LEFT].store(0.f);
        masterPeakVolume[CHANNEL_RIGHT].store(0.f);
    }
}

bool AudioProcessor::isRunning() const
{
    return running.load();
}
