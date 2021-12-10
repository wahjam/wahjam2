// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <QObject>
#include <QByteArray>
#include <vorbis/vorbisenc.h>
#include <vorbis/codec.h>

// Ogg Vorbis audio encoder using libvorbisenc
//
// Encode audio by calling encode().
class OggVorbisEncoder : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool ready READ ready)
    Q_PROPERTY(int numChannels READ numChannels)
    Q_PROPERTY(int sampleRate READ sampleRate)

public:
    OggVorbisEncoder(int numChannels, int sampleRate, QObject *parent = nullptr);
    ~OggVorbisEncoder();

    bool ready() const;
    int numChannels() const;
    int sampleRate() const;

    // Discard any state and reset the encoder
    void reset(int sampleRate = -1);

    // Encode uncompressed audio samples and return compressed audio data.
    QByteArray encode(const float *left, const float *right, size_t nsamples);

private:
    int numChannels_;
    int sampleRate_;
    bool ready_;         // successfully initialized and ready to encode?
    ogg_stream_state os;
    vorbis_info vi;
    vorbis_dsp_state vd;
    vorbis_block vb;

    bool init();
    void cleanup();
    bool writeHeader();
};

