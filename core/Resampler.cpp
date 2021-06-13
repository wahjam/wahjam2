// SPDX-License-Identifier: Apache-2.0
#include "Resampler.h"

Resampler::Resampler(QObject *parent)
    : QObject{parent}, srcState{nullptr}, ratio{1.0}, endOfInput{false}
{
    reset();
}

Resampler::~Resampler()
{
    src_delete(srcState);
}

void Resampler::reset()
{
    int error;

    src_delete(srcState);
    srcState = src_new(SRC_SINC_BEST_QUALITY, 1, &error);
    if (!srcState) {
        const char *errMsg = src_strerror(error);
        if (!errMsg) {
            errMsg = "Unkown error";
        }
        qCritical("src_new failed: %s", errMsg);
    }

    input.clear();
    ratio = 1.0;
    endOfInput = false;
}

void Resampler::setRatio(double ratio_)
{
    ratio = ratio_;
}

void Resampler::appendData(const QByteArray &data)
{
    input.append(data);
}

void Resampler::finishAppendingData()
{
    endOfInput = true;
}

size_t Resampler::resample(QByteArray *output, size_t nsamples)
{
    int oldOutputSize = output->size();
    output->resize(oldOutputSize + nsamples * sizeof(float));

    SRC_DATA srcData = {
        reinterpret_cast<float*>(input.data()),
        reinterpret_cast<float*>(output->data() + oldOutputSize),
        static_cast<long>(input.size() / sizeof(float)),
        static_cast<long>(nsamples),
        0,
        0,
        endOfInput,
        ratio,
    };

    int error = src_process(srcState, &srcData);

    input.remove(0, srcData.input_frames_used * sizeof(float));
    output->resize(oldOutputSize + srcData.output_frames_gen * sizeof(float));

    if (error) {
        const char *errMsg = src_strerror(error);
        if (!errMsg) {
            errMsg = "Unknown error";
        }
        qCritical("src_process failed: %s", errMsg);
        return 0;
    }

    return srcData.output_frames_gen;
}
