#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "AudioProcessor.h"

static const float pi = 3.14159f;

static void generateSineWave(int sampleRate, SampleTime time, float freq,
                             float *samples, size_t nsamples)
{
    const float tStep = freq / sampleRate * 2 * pi;
    float t = tStep * (time % sampleRate);

    for (size_t i = 0; i < nsamples; i++) {
        samples[i] = sinf(t);
        t += tStep;
    }
}

// Check that sine wave input is captured faithfully
static void testCapture()
{
    const int sampleRate = 44100;
    const size_t blockSize = 128;
    const float freq[] = {1000.f, 1300.f};

    AudioProcessor processor;
    float *samples[] = {
        new float[blockSize],
        new float[blockSize],
    };
    float *expectedSamples[] = {
        new float[blockSize],
        new float[blockSize],
    };

    processor.setSampleRate(sampleRate);
    processor.setRunning(true);

    for (int i = 0; i < 4; i++) {
        for (int ch = 0; ch < CHANNELS_STEREO; ch++) {
            generateSineWave(sampleRate, i * blockSize, freq[ch],
                             samples[ch], blockSize);
            memcpy(expectedSamples[ch], samples[ch],
                   blockSize * sizeof(float));
        }

        processor.process(samples, blockSize, i * blockSize);

        for (int ch = 0; ch < CHANNELS_STEREO; ch++) {
            // Check output samples first
            assert(memcmp(samples[ch], expectedSamples[ch],
                          blockSize * sizeof(float)) == 0);

            // Now check capture stream (reusing samples[] buffer)
            AudioStream &stream = processor.captureStream(ch);
            assert(stream.read(i * blockSize, samples[ch], blockSize));
            assert(memcmp(samples[ch], expectedSamples[ch],
                          blockSize * sizeof(float)) == 0);
        }
    }

    delete [] samples[CHANNEL_LEFT];
    delete [] samples[CHANNEL_RIGHT];
    delete [] expectedSamples[CHANNEL_LEFT];
    delete [] expectedSamples[CHANNEL_RIGHT];
}

static void testPlayback()
{
    // TODO
}

static void testMixing()
{
    // TODO
}

int main(int argc, char **argv)
{
    testCapture();
    testPlayback();
    testMixing();

    printf("ok\n");
    return 0;
}
