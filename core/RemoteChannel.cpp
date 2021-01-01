// SPDX-License-Identifier: Apache-2.0
#include "RemoteChannel.h"

RemoteChannel::RemoteChannel(const QString &name,
                             AppView *appView_,
                             QObject *parent)
    : QObject{parent}, appView{appView_}, name_{name}, nextPlaybackTime{0}
{
    AudioProcessor *processor = appView->audioProcessor();

    playbackStreams[CHANNEL_LEFT] = new AudioStream;
    playbackStreams[CHANNEL_RIGHT] = new AudioStream;
    processor->addPlaybackStream(playbackStreams[CHANNEL_LEFT]);
    processor->addPlaybackStream(playbackStreams[CHANNEL_RIGHT]);

    connect(appView, &AppView::processAudioStreams,
            this, &RemoteChannel::processAudioStreams);
}

RemoteChannel::~RemoteChannel()
{
    AudioProcessor *processor = appView->audioProcessor();

    processor->removePlaybackStream(playbackStreams[CHANNEL_LEFT]);
    processor->removePlaybackStream(playbackStreams[CHANNEL_RIGHT]);
}

QString RemoteChannel::name() const
{
    return name_;
}

void RemoteChannel::setName(const QString &name)
{
    name_ = name;
    emit nameChanged(name_);
}

float RemoteChannel::pan() const
{
    return 0; // TODO
}

void RemoteChannel::setPan(float pan)
{
    // TODO
}

bool RemoteChannel::monitorEnabled() const
{
    return true; // TODO
}

void RemoteChannel::setMonitorEnabled(bool enable)
{
    // TODO
}

bool RemoteChannel::underflow() const
{
    return false; // TODO
}

bool RemoteChannel::remoteSending() const
{
    return true; // TODO silence intervals?
}

// Returns true if done, false if we should try again
bool RemoteChannel::fillPlaybackStreams()
{
    if (intervals.isEmpty()){
        return true;
    }

    QByteArray left, right;
    size_t nwritable =
        qMin(playbackStreams[CHANNEL_LEFT]->numSamplesWritable(),
             playbackStreams[CHANNEL_RIGHT]->numSamplesWritable());
    auto interval = intervals.first();
    interval->setSampleRate(appView->audioProcessor()->getSampleRate());
    size_t n = interval->decode(&left, &right, nwritable);

    playbackStreams[CHANNEL_LEFT]->write(nextPlaybackTime,
            reinterpret_cast<const float*>(left.constData()),
            n);
    playbackStreams[CHANNEL_RIGHT]->write(nextPlaybackTime,
            reinterpret_cast<const float*>(right.constData()),
            n);

    nextPlaybackTime += n;

    // Finished with interval?
    if (n < nwritable && interval->appendingFinished()) {
        intervals.removeFirst();
        return false;
    }
    // TODO what if current time exceeds interval end time and download hasn't finished?

    return true;
}

void RemoteChannel::processAudioStreams()
{
    bool wasResetLeft = playbackStreams[CHANNEL_LEFT]->checkResetAndClear();
    bool wasResetRight = playbackStreams[CHANNEL_RIGHT]->checkResetAndClear();
    if (wasResetLeft || wasResetRight) {
        // TODO resync?
    }

    while (!fillPlaybackStreams()) {
        // Do nothing
    }
}

void RemoteChannel::enqueueRemoteInterval(SharedRemoteInterval remoteInterval,
                                          SampleTime nextIntervalTime)
{
    // TODO handle silent intervals
    // Start playing the first interval after the current interval
    if (intervals.isEmpty()) {
        nextPlaybackTime = nextIntervalTime;
    }
    // TODO handle BPM/BPI changes including remote intervals that are shorter/longer than the next interval

    intervals.append(remoteInterval);
}
