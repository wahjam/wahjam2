#pragma once

#include <stddef.h>
#include <stdint.h>
#include "RingBuffer.h"

// A time value, counted in audio samples
typedef uint64_t SampleTime;

enum
{
    // How many milliseconds of buffer space to preallocate
    GENEROUS_BUFFER_MSEC = 200,

    // Periodic non-real-time buffer processing interval
    SAFE_PERIODIC_TICK_MSEC = 50,
};

// Number of samples for a given duration
inline constexpr int msecToSamples(int sampleRate, int msec)
{
    return sampleRate * 1000 / msec;
}

inline void mixSamples(const float *in, float *out, size_t nsamples, float vol)
{
    while (nsamples > 0) {
        *out++ += *in++ * vol;
        nsamples--;
    }
}

// Mark a method safe to call from real-time code
#define realtime

/*
 * Audio data is transferred between real-time code and non-real-time code
 * via AudioStream.  Each stream is a single, non-interleaved channel of audio
 * samples.  Stereo audio data requires two streams: one for the left channel
 * and one for the right channel.
 */
class AudioStream
{
public:
    AudioStream(size_t sampleBufferSize = 0);
    ~AudioStream();

    // Call from non-real-time thread, discards queued data
    void setSampleBufferSize(size_t nsamples);

    // Returns true on success, false on overrun
    realtime bool write(SampleTime now, const float *samples, size_t nsamples);

    // Returns true on success, false on underrun
    realtime bool read(SampleTime now, float *samples, size_t nsamples);
    realtime bool readMix(SampleTime now, float *samples, size_t nsamples, float mixVol);

    realtime float getPan() const;
    realtime void setPan(float pan_);
    realtime bool monitorEnabled() const;
    realtime void setMonitorEnabled(bool enabled);

private:
    /*
     * Audio is transferred in a packet called AudioDescriptor.  Each descriptor
     * includes a timestamp for time synchronization.  Timestamps make it
     * possible to represent gaps in audio data.
     */
    struct AudioDescriptor
    {
        float *samples;
        size_t nsamples;
        SampleTime time;
    };

    RingBuffer<AudioDescriptor> ring;
    float *sampleBuffer;
    size_t sampleBufferSize;
    size_t writeIndex;
    std::atomic<size_t> samplesQueued;
    std::atomic<float> pan; // -1 - left, 0 - center,  1 - right
    std::atomic<bool> monitor; // mix into output?

    realtime bool readInternal(SampleTime now, float *samples, size_t nsamples, bool mix, float mixVol);
};

#undef realtime
