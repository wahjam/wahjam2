// SPDX-License-Identifier: Apache-2.0
#include <assert.h>
#include <stdio.h>
#include "core/LocalChannel.h"

static const int sampleRate = 44100;

// A fake IIntervalTime that can be controlled by test cases
class MockIntervalTime : public IIntervalTime
{
public:
    // Set this field to control the interval time that is reported
    SampleTime nextIntervalTime_;

    SampleTime nextIntervalTime() const
    {
        return nextIntervalTime_;
    }

    SampleTime remainingIntervalTime(SampleTime pos) const
    {
        // The test case should have updated the time before the next interval
        assert(pos < nextIntervalTime_);
        return nextIntervalTime_ - pos;
    }
};

static MockIntervalTime intervalTime;

struct ExpectedUploadData
{
    int channelIdx;
    bool zeroGuid;
    bool first;
    bool last;
    SampleTime intervalTimeStep;
};

// Global variables that the values from the LocalChannel::uploadData() signal
static std::vector<ExpectedUploadData> expectedUploadData;

static void uploadData(int channelIdx, const QUuid &guid,
                       const QByteArray &data, bool first, bool last)
{
    // Unexpected signal
    assert(!expectedUploadData.empty());

    auto expected = expectedUploadData.front();
    assert(expected.channelIdx == channelIdx);
    assert(expected.zeroGuid == guid.isNull());
    assert(expected.zeroGuid == data.isEmpty());
    assert(expected.first == first);
    assert(expected.last == last);

    intervalTime.nextIntervalTime_ += expected.intervalTimeStep;

    expectedUploadData.erase(expectedUploadData.begin());
}

static void generateAudioSamples(AudioStream *stream, SampleTime now,
                                 size_t nsamples)
{
    std::vector<float> samples(nsamples, 0.f);
    stream->write(now, samples.data(), nsamples);
}

static void testSilentIntervals()
{
    intervalTime.nextIntervalTime_ = sampleRate; // 1 second

    size_t sampleBufferSize = msecToSamples(sampleRate, 1000);
    AudioStream captureLeft{sampleBufferSize};
    AudioStream captureRight{sampleBufferSize};

    SampleTime now = 0;
    LocalChannel chan{"channel0", 0, &captureLeft, &captureRight, sampleRate,
                      now, &intervalTime};
    QObject::connect(&chan, &LocalChannel::uploadData, uploadData);
    chan.setSend(false);

    // The first interval has no signals
    generateAudioSamples(&captureLeft, now, sampleBufferSize - 1);
    generateAudioSamples(&captureRight, now, sampleBufferSize - 1);
    now += sampleBufferSize - 1;
    chan.processAudioStreams();

    // The second interval has one signal
    expectedUploadData.push_back((ExpectedUploadData){
        .channelIdx = 0,
        .zeroGuid = true,
        .first = true,
        .last = true,
        .intervalTimeStep = sampleRate,
    });
    generateAudioSamples(&captureLeft, now, 1);
    generateAudioSamples(&captureRight, now, 1);
    now += 1;
    chan.processAudioStreams();
    assert(expectedUploadData.empty());

    // The third interval has one signal but three processAudioStreams() calls
    expectedUploadData.push_back((ExpectedUploadData){
        .channelIdx = 0,
        .zeroGuid = true,
        .first = true,
        .last = true,
        .intervalTimeStep = sampleRate,
    });
    generateAudioSamples(&captureLeft, now, sampleBufferSize);
    generateAudioSamples(&captureRight, now, sampleBufferSize);
    now += sampleBufferSize;
    chan.processAudioStreams();
    assert(expectedUploadData.empty());

    generateAudioSamples(&captureLeft, now, sampleBufferSize / 2);
    generateAudioSamples(&captureRight, now, sampleBufferSize / 2);
    now += sampleBufferSize / 2;
    chan.processAudioStreams();

    generateAudioSamples(&captureLeft, now, sampleBufferSize / 2 - 1);
    generateAudioSamples(&captureRight, now, sampleBufferSize / 2 - 1);
    now += sampleBufferSize / 2 - 1;
    chan.processAudioStreams();
}

static void testSendIntervals()
{
    intervalTime.nextIntervalTime_ = sampleRate; // 1 second

    size_t sampleBufferSize = msecToSamples(sampleRate, 1000);
    AudioStream captureLeft{sampleBufferSize};
    AudioStream captureRight{sampleBufferSize};

    SampleTime now = 0;
    LocalChannel chan{"channel0", 0, &captureLeft, &captureRight, sampleRate,
                      now, &intervalTime};
    QObject::connect(&chan, &LocalChannel::uploadData, uploadData);

    // The first interval has no signals
    generateAudioSamples(&captureLeft, now, sampleBufferSize);
    generateAudioSamples(&captureRight, now, sampleBufferSize);
    now += sampleBufferSize;
    chan.processAudioStreams();
    intervalTime.nextIntervalTime_ += sampleRate;

    // The second interval has one signal
    expectedUploadData.push_back((ExpectedUploadData){
        .channelIdx = 0,
        .zeroGuid = false,
        .first = true,
        .last = true,
        .intervalTimeStep = sampleRate,
    });
    generateAudioSamples(&captureLeft, now, sampleBufferSize);
    generateAudioSamples(&captureRight, now, sampleBufferSize);
    now += sampleBufferSize;
    chan.processAudioStreams();
    assert(expectedUploadData.empty());

    // The third interval
    expectedUploadData.push_back((ExpectedUploadData){
        .channelIdx = 0,
        .zeroGuid = false,
        .first = true,
        .last = false,
        .intervalTimeStep = 0,
    });
    generateAudioSamples(&captureLeft, now, sampleBufferSize / 3);
    generateAudioSamples(&captureRight, now, sampleBufferSize / 3);
    now += sampleBufferSize / 3;
    chan.processAudioStreams();
    assert(expectedUploadData.empty());

    expectedUploadData.push_back((ExpectedUploadData){
        .channelIdx = 0,
        .zeroGuid = false,
        .first = false,
        .last = false,
        .intervalTimeStep = 0,
    });
    generateAudioSamples(&captureLeft, now, sampleBufferSize / 3);
    generateAudioSamples(&captureRight, now, sampleBufferSize / 3);
    now += sampleBufferSize / 3;
    chan.processAudioStreams();
    assert(expectedUploadData.empty());

    expectedUploadData.push_back((ExpectedUploadData){
        .channelIdx = 0,
        .zeroGuid = false,
        .first = false,
        .last = true,
        .intervalTimeStep = sampleRate,
    });
    generateAudioSamples(&captureLeft, now, sampleBufferSize / 3);
    generateAudioSamples(&captureRight, now, sampleBufferSize / 3);
    now += sampleBufferSize / 3;
    chan.processAudioStreams();
    assert(expectedUploadData.empty());
}

int main(int argc, char **argv)
{
    testSilentIntervals();
    testSendIntervals();

    printf("ok\n");
    return 0;
}
