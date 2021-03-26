// SPDX-License-Identifier: Apache-2.0
#include <assert.h>
#include <stdio.h>
#include "core/OggVorbisEncoder.h"

enum {
    NUM_CHANNELS = 1,
    SAMPLE_RATE = 44100, /* Hz */
};

static void testReset()
{
    OggVorbisEncoder encoder(NUM_CHANNELS, SAMPLE_RATE);

    std::vector<float> samples(SAMPLE_RATE, 0.f);

    QByteArray data = encoder.encode(samples.data(), nullptr, samples.size());
    data.append(encoder.encode(nullptr, nullptr, 0));
    assert(data.size() > 1);

    encoder.reset();

    data = encoder.encode(samples.data(), nullptr, samples.size());
    data.append(encoder.encode(nullptr, nullptr, 0));
    assert(data.size() > 1);
}

int main(int argc, char **argv)
{
    testReset();

    printf("ok\n");
    return 0;
}
