// SPDX-License-Identifier: Apache-2.0
#include <errno.h>
#include <QFile>
#include "OggVorbisDecoder.h"

OggVorbisDecoder::OggVorbisDecoder(QObject *parent)
    : QObject(parent), state_{State::Closed}
{
}

OggVorbisDecoder::~OggVorbisDecoder()
{
    reset();
}

void OggVorbisDecoder::reset()
{
    input.clear();

    if (state_ != State::Closed) {
        ov_clear(&ovfile);
        state_ = State::Closed;
    }
}

OggVorbisDecoder::State OggVorbisDecoder::state() const
{
    return state_;
}

// Not const because ov_info() takes a non-const argument
int OggVorbisDecoder::sampleRate()
{
    if (state_ == State::Open) {
        vorbis_info *info = ov_info(&ovfile, -1);

        if (info) {
            return info->rate;
        }
    }

    return 0;
}

// Read nmemb elements of size from input into the buffer at ptr
size_t OggVorbisDecoder::readFunc(void *ptr, size_t size, size_t nmemb)
{
    // Calculate how many elements to read
    size_t n = qMin(nmemb, input.size() / size);
    size_t nbytes = n * size;

    memcpy(ptr, input.data(), nbytes);
    input.remove(0, nbytes);

    // Clear errno because libvorbisfile checks it when 0 is returned
    errno = 0;

    return n;
}

// This is the trampoline function
size_t OggVorbisDecoder::readFunc_(void *ptr, size_t size, size_t nmemb,
                                   void *datasource)
{
    OggVorbisDecoder *this_ = reinterpret_cast<OggVorbisDecoder*>(datasource);

    return this_->readFunc(ptr, size, nmemb);
}

// Returns the number of audio channels (1 or 2), or -1 on failure
static int getNumChannels(OggVorbis_File *ovfile)
{
    vorbis_info *info = ov_info(ovfile, -1);
    if (!info) {
        qCritical("Cannot fetch vorbis_info");
        return -1;
    }
    if (info->channels != 1 && info->channels != 2) {
        qCritical("Expected mono or stereo, got %d channels", info->channels);
        return -1;
    }
    return info->channels;
}

// Ensure we're in the open state or return false
bool OggVorbisDecoder::tryOpen()
{
    if (state_ == State::Open) {
        return true;
    }
    if (state_ == State::Error) {
        return false;
    }

    ov_callbacks callbacks = {
        &OggVorbisDecoder::readFunc_,
        nullptr,
        nullptr,
        nullptr,
    };
    int ret;

    // ov_open_callbacks() will completely consume input but we'll also need to
    // restore it in case of failure.
    QByteArray peekData{std::move(input)};

    ret = ov_open_callbacks(this, &ovfile, peekData.data(), peekData.size(),
                            callbacks);
    if (ret < 0) {
        // Restore input so caller can try opening again later
        input = std::move(peekData);
        return false;
    }

    state_ = State::Open;

    // Check our assumptions about the input file
    long nstreams = ov_streams(&ovfile);
    if (nstreams != 1) {
        qCritical("Expected 1 logical bitstream, got %ld", nstreams);
        reset();
        return false;
    }
    if (getNumChannels(&ovfile) == -1) {
        reset();
        return false;
    }

    return true;
}

static ssize_t doDecode(OggVorbis_File *ovfile,
                        QByteArray *left,
                        QByteArray *right,
                        size_t nsamples)
{
    float **samples;
    int bitstream;
    long n = ov_read_float(ovfile, &samples, nsamples, &bitstream);
    if (n == 0) {
        return 0;
    } else if (n < 0) {
        return -1;
    }

    // Check the number of channels after reading, it reflects the most
    // up-to-date value
    int nchannels = getNumChannels(ovfile);
    if (nchannels == -1) {
        return -1;
    }

    // TODO stereo pan law?
    left->append(reinterpret_cast<char*>(samples[0]),
                 n * sizeof(samples[0][0]));
    if (nchannels == 1) {
        right->append(reinterpret_cast<char*>(samples[0]),
                      n * sizeof(samples[0][0]));
    } else {
        right->append(reinterpret_cast<char*>(samples[1]),
                      n * sizeof(samples[1][0]));
    }

    return n;
}

void OggVorbisDecoder::appendData(const QByteArray &data)
{
    input.append(data);
}

size_t OggVorbisDecoder::decode(QByteArray *left, QByteArray *right,
                                size_t nsamples)
{
    if (!tryOpen()) {
        return 0;
    }

    size_t decoded = 0;
    while (decoded < nsamples) {
        ssize_t n = doDecode(&ovfile, left, right, nsamples - decoded);
        if (n <= 0) {
            break;
        }

        decoded += n;
    }

    return decoded;
}

size_t OggVorbisDecoder::decodeFile(const char *filename,
                                    QByteArray *left,
                                    QByteArray *right,
                                    int *sampleRate)
{
    QFile file{filename};
    if (!file.open(QIODevice::ReadOnly)) {
        qCritical("Failed to open file \"%s\"", filename);
        return false;
    }

    OggVorbisDecoder decoder;
    decoder.appendData(file.readAll());

    bool gotSampleRate = false;
    size_t nsamples = 0;
    size_t n;
    while ((n = decoder.decode(left, right, 8192)) > 0) {
        if (gotSampleRate) {
            if (*sampleRate != decoder.sampleRate()) {
                qCritical("Unexpected sample rate change in Ogg Vorbis file \"%s\"", filename);
                return false;
            }
        } else {
            *sampleRate = decoder.sampleRate();
            gotSampleRate = true;
        }

        nsamples += n;
    }

    return nsamples;
}
