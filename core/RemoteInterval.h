// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "audio/AudioStream.h"
#include "JamConnection.h"
#include "OggVorbisDecoder.h"
#include "Resampler.h"

// An interval of compressed audio data being downloaded from the server
//
// Call appendData() each time compressed audio data is received. Call
// finishAppendingData() when the download is complete.
//
// Decode audio samples by calling decode(). The download may still be in
// progress and if there is not enough data fewer samples than requested will
// be returned.
class RemoteInterval : public QObject
{
    Q_OBJECT

public:
    RemoteInterval(const QString &username,
                   const QUuid &guid,
                   const JamConnection::FourCC fourCC,
                   QObject *parent = nullptr);

    // This must be called before decode()
    void setResampler(Resampler *left, Resampler *right);

    QString username() const;
    QUuid guid() const;

    // Is this a silent interval with no audio data?
    bool isSilence() const;

    // May be called at any time to change the sample rate
    void setSampleRate(int rate);

    // Returns true if no more samples can be decoded after decode() returns 0
    bool appendingFinished() const;

    // Fill the stereo left/right channels with up to nsamples of decoded
    // samples. Returns the number of samples decoded.
    size_t decode(QByteArray *left, QByteArray *right, size_t nsamples);

public slots:
    // Add compressed audio data
    void appendData(const QByteArray &data);

    // No more compressed audio data will be appended
    void finishAppendingData();

private:
    OggVorbisDecoder decoder;
    Resampler *resampler[CHANNELS_STEREO];
    QString username_;
    QUuid guid_;
    JamConnection::FourCC fourCC;
    int outputSampleRate;
    bool decodeStarted;
    bool finished;

    size_t drainResampler(QByteArray *left, QByteArray *right,
                          size_t nsamples);
    size_t fillResampler(size_t nsamples);
};
