// SPDX-License-Identifier: Apache-2.0
#include "RemoteInterval.h"

RemoteInterval::RemoteInterval(const QString &username,
                               const QUuid &guid,
                               const JamConnection::FourCC fourCC_,
                               QObject *parent)
    : QObject{parent}, username_{username}, guid_{guid},
      outputSampleRate{44100}, decodeStarted{false}, finished{false}
{
    memcpy(fourCC, fourCC_, sizeof(fourCC));
}

QString RemoteInterval::username() const
{
    return username_;
}

QUuid RemoteInterval::guid() const
{
    return guid_;
}

void RemoteInterval::setSampleRate(int rate)
{
    outputSampleRate = rate;
}

bool RemoteInterval::appendingFinished() const
{
    return finished;
}

// Returns number of output samples
size_t RemoteInterval::drainResampler(QByteArray *left, QByteArray *right,
                                      size_t nsamples)
{
    size_t n = resampler[CHANNEL_LEFT].resample(left, nsamples);
    size_t m = resampler[CHANNEL_RIGHT].resample(right, n);
    if (n != m) {
        qWarning("Stereo channels out of sync, resamplers produced %zu and %zu samples",
                 n, m);
    }
    return qMin(n, m);
}

// Returns number of samples filled
size_t RemoteInterval::fillResampler(size_t nsamples)
{
    QByteArray tmpLeft, tmpRight;

    // Estimate how many input samples need to be decoded to produce nsamples
    // output samples.
    int inputSampleRate = decodeStarted ? decoder.sampleRate() : 44100;
    size_t inputSamples = nsamples *
                          static_cast<double>(inputSampleRate) /
                          outputSampleRate + 0.5;

    size_t n = decoder.decode(&tmpLeft, &tmpRight, inputSamples);
    if (n > 0) {
        double ratio = static_cast<double>(outputSampleRate) /
                       decoder.sampleRate();
        resampler[CHANNEL_LEFT].setRatio(ratio);
        resampler[CHANNEL_RIGHT].setRatio(ratio);
        decodeStarted = true;
    }

    // We have no input samples yet, so there is nothing to resample
    if (!decodeStarted) {
        return 0;
    }

    resampler[CHANNEL_LEFT].appendData(tmpLeft);
    resampler[CHANNEL_RIGHT].appendData(tmpRight);
    return n;
}

size_t RemoteInterval::decode(QByteArray *left, QByteArray *right,
                              size_t nsamples)
{
    bool needFill = false;
    size_t decoded = 0;

    while (nsamples > 0) {
        size_t filled = 0;
        if (needFill) {
            filled = fillResampler(nsamples);
        }

        size_t n = drainResampler(left, right, nsamples);

        // No input left to decode, stop for now
        if (n == 0 && needFill && filled == 0) {
            break;
        }

        nsamples -= n;
        decoded += n;
        needFill = true;
    }

    return decoded;
}

void RemoteInterval::appendData(const QByteArray &data)
{
    decoder.appendData(data);
}

void RemoteInterval::finishAppendingData()
{
    resampler[CHANNEL_LEFT].finishAppendingData();
    resampler[CHANNEL_RIGHT].finishAppendingData();
    finished = true;
}
