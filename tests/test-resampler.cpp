// SPDX-License-Identifier: Apache-2.0
#include <assert.h>
#include <stdio.h>
#include <QFile>
#include "core/OggVorbisDecoder.h"
#include "core/Resampler.h"

static void resample(const char *filename, int seconds, int inputSampleRate,
                     int outputSampleRate)
{
    QByteArray left, right;
    int actualSampleRate;
    size_t n = OggVorbisDecoder::decodeFile(filename, &left, &right,
                                            &actualSampleRate);
    size_t expectedInputSamples = seconds * inputSampleRate;
    assert(n == expectedInputSamples);
    assert(actualSampleRate == inputSampleRate);

    Resampler resampler;
    resampler.setRatio(static_cast<double>(outputSampleRate) /
                       inputSampleRate);
    resampler.appendData(left);
    resampler.finishAppendingData();

    QByteArray output;
    size_t expectedOutputSamples = seconds * outputSampleRate;
    n = resampler.resample(&output, expectedOutputSamples);
    if (inputSampleRate > outputSampleRate) {
        assert(n < expectedInputSamples);
    } else {
        assert(n > expectedInputSamples);
    }
    assert(static_cast<size_t>(output.size()) == n * sizeof(float));
}

static void testUpsample()
{
    resample("data/sine-44_1kHz-mono.ogg", 8, 44100, 48000);
}

static void testDownsample()
{
    resample("data/sine-48kHz-mono.ogg", 8, 48000, 44100);
}

int main(int argc, char **argv)
{
    testUpsample();
    testDownsample();

    printf("ok\n");
    return 0;
}
