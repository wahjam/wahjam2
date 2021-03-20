// SPDX-License-Identifier: Apache-2.0
#include "OggVorbisEncoder.h"
#include <vorbis/codec.h>
#include <vorbis/vorbisenc.h>

OggVorbisEncoder::OggVorbisEncoder(int numChannels, int sampleRate,
                                   QObject *parent)
    : QObject{parent}, numChannels_{numChannels}, sampleRate_{sampleRate},
      ready_{false}
{
    assert(numChannels == 1 || numChannels == 2);
    ready_ = init();
}

OggVorbisEncoder::~OggVorbisEncoder()
{
    if (ready_) {
        cleanup();
    }
}

bool OggVorbisEncoder::ready() const
{
    return ready_;
}

int OggVorbisEncoder::numChannels() const
{
    return numChannels_;
}

int OggVorbisEncoder::sampleRate() const
{
    return sampleRate_;
}

// Returns true on success, false otherwise
bool OggVorbisEncoder::init()
{
    int ret;
    int serialno = 0; // unique id for our one and only stream

    ret = ogg_stream_init(&os, serialno);
    if (ret != 0) {
        qWarning("ogg_stream_init failed %d", ret);
        return false;
    }

    vorbis_info_init(&vi);

    float quality = 0.f; // nominal bitrate ~64 kbps
    ret = vorbis_encode_init_vbr(&vi, numChannels_, sampleRate_, quality);
    if (ret != 0) {
        qWarning("vorbis_encoder_init_vbr failed %d", ret);
        ogg_stream_clear(&os);
        return false;
    }

    ret = vorbis_analysis_init(&vd, &vi);
    if (ret != 0) {
        qWarning("vorbis_analysis_init failed %d", ret);
        vorbis_info_clear(&vi);
        ogg_stream_clear(&os);
        return false;
    }

    if (!writeHeader()) {
        vorbis_dsp_clear(&vd);
        vorbis_info_clear(&vi);
        ogg_stream_clear(&os);
        return false;
    }

    ret = vorbis_block_init(&vd, &vb);
    if (ret != 0) {
        qWarning("vorbis_block_init failed %d", ret);
        vorbis_dsp_clear(&vd);
        vorbis_info_clear(&vi);
        ogg_stream_clear(&os);
        return false;
    }

    return true;
}

void OggVorbisEncoder::cleanup()
{
    vorbis_block_clear(&vb);
    vorbis_dsp_clear(&vd);
    vorbis_info_clear(&vi);
    ogg_stream_clear(&os);
}

void OggVorbisEncoder::reset()
{
    if (ready_) {
        cleanup();
    }
    ready_ = init();
}

// Returns true on success, false otherwise
bool OggVorbisEncoder::writeHeader()
{
    ogg_packet op;
    ogg_packet op_comm;
    ogg_packet op_code;
    vorbis_comment vc;
    int ret;

    vorbis_comment_init(&vc);
    ret = vorbis_analysis_headerout(&vd, &vc, &op, &op_comm, &op_code);
    vorbis_comment_clear(&vc);
    if (ret != 0) {
        qWarning("vorbis_analysis_headerout failed %d", ret);
        return false;
    }

    bool ok = ogg_stream_packetin(&os, &op) == 0 &&
              ogg_stream_packetin(&os, &op_comm) == 0 &&
              ogg_stream_packetin(&os, &op_code) == 0;

    if (!ok) {
        qWarning("ogg_stream_packetin failed for header");
    }
    return ok;
}

QByteArray OggVorbisEncoder::encode(const float *left,
                                    const float *right,
                                    size_t nsamples)
{
    QByteArray output;
    int ret;

    if (!ready_) {
        return output;
    }

    if (left) {
        if (numChannels_ == 1) {
            assert(!right);
        } else {
            assert(right);
        }

        float **samples = vorbis_analysis_buffer(&vd, nsamples);

        memcpy(samples[0], left, nsamples * sizeof(float));
        if (right) {
            memcpy(samples[1], right, nsamples * sizeof(float));
        }

        ret = vorbis_analysis_wrote(&vd, nsamples);
        if (ret != 0) {
            qWarning("vorbis_analysis_wrote nsamples %zu failed %d",
                     nsamples, ret);
            return output;
        }
    } else {
        assert(!right);
        vorbis_analysis_wrote(&vd, 0); // end of stream
    }

    bool moreBlocks;
    do {
        ret = vorbis_analysis_blockout(&vd, &vb);
        moreBlocks = ret == 1;
        if (ret < 0) {
            qWarning("vorbis_analysis_blockout failed %d", ret);
            return output;
        }

        ret = vorbis_analysis(&vb, nullptr);
        if (ret < 0) {
            qWarning("vorbis_analysis failed %d", ret);
            return output;
        }

        ret = vorbis_bitrate_addblock(&vb);
        if (ret < 0) {
            qWarning("vorbis_bitrate_addblock failed %d", ret);
            return output;
        }

        ogg_packet op;
        bool morePackets;
        do {
            ret = vorbis_bitrate_flushpacket(&vd, &op);
            morePackets = ret == 1;
            if (ret < 0) {
                qWarning("vorbis_bitrate_flushpacket failed %d", ret);
                return output;
            }

            ret = ogg_stream_packetin(&os, &op);
            if (ret < 0) {
                qWarning("ogg_stream_packetin failed %d", ret);
                return output;
            }

            ogg_page og;
            while (ogg_stream_pageout(&os, &og) != 0) {
                output.append(reinterpret_cast<const char*>(og.header),
                              static_cast<int>(og.header_len));
                output.append(reinterpret_cast<const char*>(og.body),
                              static_cast<int>(og.body_len));
            }
        } while (morePackets);
    } while (moreBlocks);

    return output;
}
