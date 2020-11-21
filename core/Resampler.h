// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <QObject>
#include <QByteArray>
#include <samplerate.h>

// Sample rate converter using libsamplerate
class Resampler : public QObject
{
    Q_OBJECT

public:
    // The sampling conversion ratio is outputSampleRate / inputSampleRate
    Resampler(QObject *parent = nullptr);
    ~Resampler();

    void setRatio(double ratio);

    // Discard any state and reset the resampler
    void reset();

    // Fill output with up to nsamples of resampled audio data. Returns the
    // number of samples converted or 0 if no more samples are available. More
    // input audio data may be added with appendData() to resume conversion
    // after 0 was returned.
    size_t resample(QByteArray *output, size_t nsamples);

public slots:
    // Add input audio data
    void appendData(const QByteArray &data);

    // No more input audio data will be appended
    void finishAppendingData();

private:
    QByteArray input;
    SRC_STATE *srcState;
    double ratio;
    bool endOfInput;
};
