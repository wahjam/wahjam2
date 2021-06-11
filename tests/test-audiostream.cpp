// SPDX-License-Identifier: Apache-2.0
#include <stdio.h>
#include <string.h>
#include "audio/AudioStream.h"

const size_t blockSize = 64 /* samples */;

void testWriteFull()
{
    AudioStream stream{AudioStream::PLAYBACK, 4 * blockSize};
    float samples[blockSize] = {};

    for (int i = 0; i < 4; i++) {
        assert(stream.write(i * blockSize, samples, blockSize) == blockSize);
    }

    assert(stream.write(5 * blockSize, samples, blockSize) == 0);
}

void testReadEmpty()
{
    AudioStream stream{AudioStream::PLAYBACK, 4 * blockSize};
    float samples[blockSize] = {};

    assert(stream.read(0, samples, blockSize) == 0);
}

void testWrapBuffer()
{
    float samples[blockSize];

    AudioStream stream{AudioStream::PLAYBACK, 4 * blockSize};

    // Almost fill buffer
    const size_t nzeroes = 3 * blockSize + blockSize / 2;
    const float zeroes[nzeroes] = {};
    assert(stream.write(0, zeroes, nzeroes) == nzeroes);

    // Dequeue data the start of the buffer
    assert(stream.read(0, samples, blockSize / 2) == blockSize / 2);
    assert(memcmp(samples, zeroes, blockSize / 2 * sizeof(float)) == 0);

    // Wrap a unique pattern around the end of the buffer
    float pattern[blockSize];
    for (size_t i = 0; i < blockSize; i++) {
        pattern[i] = i;
    }
    assert(stream.write(nzeroes, pattern, blockSize) == blockSize);

    // Dequeue rest of zeroes
    for (int i = 0; i < 3; i++) {
        assert(stream.read(blockSize / 2 + i * blockSize, samples,
                           blockSize) == blockSize);
        assert(memcmp(samples, zeroes, sizeof(samples)) == 0);
    }

    // Dequeue unique pattern around the end of the buffer
    assert(stream.read(nzeroes, samples, blockSize) == blockSize);
    assert(memcmp(samples, pattern, sizeof(samples)) == 0);
}

void testNumSamplesWritable()
{
    AudioStream stream{AudioStream::PLAYBACK, 4 * blockSize};
    float samples[blockSize] = {};

    assert(stream.numSamplesWritable() == 4 * blockSize);
    assert(stream.numSamplesReadable() == 0);

    assert(stream.write(0, samples, blockSize) == blockSize);
    assert(stream.numSamplesWritable() == 3 * blockSize);
    assert(stream.numSamplesReadable() == blockSize);

    assert(stream.write(blockSize, samples, blockSize) == blockSize);
    assert(stream.numSamplesWritable() == 2 * blockSize);
    assert(stream.numSamplesReadable() == 2 * blockSize);

    assert(stream.read(0, samples, blockSize) == blockSize);
    assert(stream.numSamplesWritable() == 3 * blockSize);
    assert(stream.numSamplesReadable() == blockSize);

    assert(stream.read(blockSize, samples, blockSize) == blockSize);
    assert(stream.numSamplesWritable() == 4 * blockSize);
    assert(stream.numSamplesReadable() == 0);

    assert(stream.write(2 * blockSize, samples, blockSize) == blockSize);
    assert(stream.write(3 * blockSize, samples, blockSize) == blockSize);
    assert(stream.write(4 * blockSize, samples, blockSize) == blockSize);
    assert(stream.write(5 * blockSize, samples, blockSize) == blockSize);
    assert(stream.numSamplesWritable() == 0);
    assert(stream.numSamplesReadable() == 4 * blockSize);

    assert(stream.read(2 * blockSize, samples, blockSize) == blockSize);
    assert(stream.read(3 * blockSize, samples, blockSize) == blockSize);
    assert(stream.read(4 * blockSize, samples, blockSize) == blockSize);
    assert(stream.read(5 * blockSize, samples, blockSize) == blockSize);
    assert(stream.numSamplesWritable() == 4 * blockSize);
    assert(stream.numSamplesReadable() == 0);
}

int main(int argc, char **argv)
{
    testWriteFull();
    testReadEmpty();
    testWrapBuffer();
    testNumSamplesWritable();

    printf("ok\n");
    return 0;
}
