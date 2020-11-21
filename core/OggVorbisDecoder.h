// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <QObject>
#include <QByteArray>
#include <vorbis/vorbisfile.h>

// Ogg Vorbis audio decoder using libvorbisfile
//
// Call appendData() each time compressed audio data is received.
//
// Decode audio samples by calling decode(). If there is not enough compressed
// audio data fewer samples than requested will be returned.
class OggVorbisDecoder : public QObject
{
    Q_OBJECT
    Q_PROPERTY(State state READ state)
    Q_PROPERTY(int sampleRate READ sampleRate)

public:
    enum State {
        Closed,
        Open,
        Error
    };
    Q_ENUM(State)

    OggVorbisDecoder(QObject *parent = nullptr);
    ~OggVorbisDecoder();

    // Discard any state and reset the decoder
    void reset();

    State state() const;

    // In theory the sample rate can change after each decode()! The caller
    // should handle that just to be safe.
    int sampleRate();

    // Fill the stereo left/right channels with up to nsamples of decoded
    // samples. Returns the number of samples decoded or 0 if no more samples
    // are available. More compressed audio data may be added with appendData()
    // to resume decoding after 0 was returned.
    size_t decode(QByteArray *left, QByteArray *right, size_t nsamples);

    // One-shot convenience function to decode a whole file. Returns the number
    // of samples decoded or 0 if there are an error.
    static size_t decodeFile(const char *filename,
                             QByteArray *left,
                             QByteArray *right,
                             int *sampleRate);

public slots:
    // Add compressed audio data
    void appendData(const QByteArray &data);

private:
    OggVorbis_File ovfile;
    QByteArray input;
    State state_;

    size_t readFunc(void *ptr, size_t size, size_t nmemb);
    static size_t readFunc_(void *ptr, size_t size, size_t nmemb,
                            void *datasource);
    bool tryOpen();
};
