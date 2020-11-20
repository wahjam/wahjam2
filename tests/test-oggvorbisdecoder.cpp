// SPDX-License-Identifier: Apache-2.0
#include <assert.h>
#include <stdio.h>
#include <QFile>
#include "core/OggVorbisDecoder.h"

static void testEmptyInput()
{
    QByteArray left, right;
    OggVorbisDecoder decoder;

    assert(decoder.state() == OggVorbisDecoder::State::Closed);
    assert(decoder.sampleRate() == 0);

    assert(decoder.decode(&left, &right, 128) == 0);

    assert(decoder.state() == OggVorbisDecoder::State::Closed);
    assert(decoder.sampleRate() == 0);

    decoder.reset();

    assert(decoder.state() == OggVorbisDecoder::State::Closed);
    assert(decoder.sampleRate() == 0);
}

// Decode a file of a given duration and sample rate
static void decodeFile(const char *filename, int seconds, int sampleRate)
{
    QFile file{filename};
    assert(file.open(QIODevice::ReadOnly));

    OggVorbisDecoder decoder;
    decoder.appendData(file.readAll());

    QByteArray left, right;
    size_t nsamples = seconds * sampleRate;
    size_t bufSize = 128;
    size_t i;

    for (i = 0; i < nsamples; i += bufSize) {
        size_t expected = qMin(bufSize, nsamples - i);
        assert(decoder.decode(&left, &right, expected) == expected);
        assert(decoder.state() == OggVorbisDecoder::State::Open);
        assert(decoder.sampleRate() == sampleRate);
    }

    int nbytes = sizeof(float) * nsamples;
    assert(left.size() == nbytes);
    assert(right.size() == nbytes);
}

static void testMono()
{
    decodeFile("data/sine-44_1kHz-mono.ogg", 8, 44100);
}

static void testStereo()
{
    decodeFile("data/sine-44_1kHz-stereo.ogg", 8, 44100);
}

// Decode a file with small reads that may not cover a full packet
static void decodeFileSmallReads(const char *filename, int seconds,
                                 int sampleRate)
{
    QFile file{filename};
    assert(file.open(QIODevice::ReadOnly));

    QByteArray left, right;
    OggVorbisDecoder decoder;
    int remaining = seconds * sampleRate;

    while (remaining > 0) {
        int nread = decoder.decode(&left, &right, 128);
        if (nread == 0) {
            QByteArray input = file.read(32);
            decoder.appendData(input);
            assert(input.size() > 0);
            continue;
        }

        assert(nread > 0);
        assert(nread <= remaining);
        assert(decoder.state() == OggVorbisDecoder::State::Open);
        assert(decoder.sampleRate() == sampleRate);

        remaining -= nread;
    }

    int nbytes = sizeof(float) * seconds * sampleRate;
    assert(left.size() == nbytes);
    assert(right.size() == nbytes);
}

static void testSmallReadsMono()
{
    decodeFileSmallReads("data/sine-44_1kHz-mono.ogg", 8, 44100);
}

static void testSmallReadsStereo()
{
    decodeFileSmallReads("data/sine-44_1kHz-stereo.ogg", 8, 44100);
}

int main(int argc, char **argv)
{
    testEmptyInput();
    testMono();
    testStereo();
    testSmallReadsMono();
    testSmallReadsStereo();

    printf("ok\n");
    return 0;
}
