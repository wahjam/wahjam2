#include <stdio.h>
#include <string.h>
#include "AudioStream.h"

const size_t blockSize = 64 /* samples */;

void testWriteFull()
{
    AudioStream stream{4 * blockSize};
    float samples[blockSize] = {};

    for (int i = 0; i < 4; i++) {
        assert(stream.write(i * blockSize, samples, blockSize));
    }

    assert(!stream.write(5 * blockSize, samples, blockSize));
}

void testReadEmpty()
{
    AudioStream stream{4 * blockSize};
    float samples[blockSize] = {};

    assert(!stream.read(0, samples, blockSize));
}

void testWrapBuffer()
{
    float samples[blockSize];

    AudioStream stream{4 * blockSize};

    // Almost fill buffer
    const size_t nzeroes = 3 * blockSize + blockSize / 2;
    const float zeroes[nzeroes] = {};
    assert(stream.write(0, zeroes, blockSize / 2));
    assert(stream.write(blockSize / 2, zeroes, 3 * blockSize));

    // Dequeue data the start of the buffer
    assert(stream.read(0, samples, blockSize / 2));
    assert(memcmp(samples, zeroes, blockSize / 2 * sizeof(float)) == 0);

    // Wrap a unique pattern around the end of the buffer
    float pattern[blockSize];
    for (size_t i = 0; i < blockSize; i++) {
        pattern[i] = i;
    }
    assert(stream.write(nzeroes, pattern, blockSize));

    // Dequeue rest of zeroes
    for (int i = 0; i < 3; i++) {
        assert(stream.read(blockSize / 2 + i * blockSize, samples, blockSize));
        assert(memcmp(samples, zeroes, sizeof(samples)) == 0);
    }

    // Dequeue unique pattern around the end of the buffer
    assert(stream.read(nzeroes, samples, blockSize));
    assert(memcmp(samples, pattern, sizeof(samples)) == 0);
}

int main(int argc, char **argv)
{
    testWriteFull();
    testReadEmpty();
    testWrapBuffer();

    printf("ok\n");
    return 0;
}
