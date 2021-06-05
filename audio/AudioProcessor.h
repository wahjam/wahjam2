// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "rcu.h"
#include "AudioStream.h"

// Mark a method safe to call from real-time code
#define realtime

/*
 * The AudioProcessor is the real-time audio engine that mixes playback streams
 * with input audio to produce output audio.  Input audio is also made
 * available as capture streams for VU meters, analysis, etc.
 *
 * The real-time audio thread must call process() with samples.
 *
 * The non-realtime thread owns the AudioProcessor.  It calls setRunning() to
 * enable/disable stream processing.  It calls tick() periodically.  It reads
 * audio from capture streams and writes audio to playback streams.
 */
class AudioProcessor
{
public:
    AudioProcessor();
    ~AudioProcessor();

    // Shares ownership of stream
    void addPlaybackStream(AudioStream *stream);

    // Takes ownership of stream
    void removePlaybackStream(AudioStream *stream);

    AudioStream &captureStream(int channel);

    // Call this periodically from the non-real-time thread
    void tick();

    int getSampleRate() const;
    void setSampleRate(int rate);

    realtime float getMasterGain() const;
    realtime void setMasterGain(float gain);

    // Enable/disable processing
    void setRunning(bool enabled);
    bool isRunning() const;

    // Get next expected sample time, may be called from any thread
    realtime SampleTime getNextSampleTime() const;

    // The heart of the real-time audio processing
    realtime void process(float *inOutSamples[CHANNELS_STEREO], size_t nsamples, SampleTime now);

private:
    RCUContext rcu;

    typedef std::vector<RCUPointer<AudioStream>> PlaybackStreams;
    RCUPointer<PlaybackStreams> playbackStreams;

    AudioStream captureStreams[CHANNELS_STEREO];

    std::atomic<int> sampleRate;
    std::atomic<bool> running;
    std::atomic<SampleTime> nextSampleTime;
    std::atomic<float> masterGain;

    void setSampleBufferSize(size_t nsamples);
    void processInputs(float *inOutSamples[CHANNELS_STEREO],
                       size_t nsamples, SampleTime now);
    void mixPlaybackStreams(float *inOutSamples[CHANNELS_STEREO],
                            size_t nsamples, SampleTime now);
};

#undef realtime
