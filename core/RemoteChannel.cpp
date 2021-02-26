// SPDX-License-Identifier: Apache-2.0
#include "JamSession.h"
#include "QmlGlobals.h"
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

    // TODO handle underflow when remote interval falls behind actual playback position

    JamSession *session = appView->qmlGlobals()->session();
    QByteArray left, right;
    size_t nwritable =
        qMin(playbackStreams[CHANNEL_LEFT]->numSamplesWritable(),
             playbackStreams[CHANNEL_RIGHT]->numSamplesWritable());
    auto interval = intervals.first();
    SampleTime remaining = session->remainingIntervalTime(nextPlaybackTime);
    interval->setSampleRate(appView->audioProcessor()->getSampleRate());
    size_t n = interval->decode(&left, &right, qMin(nwritable, remaining));

    playbackStreams[CHANNEL_LEFT]->write(nextPlaybackTime,
            reinterpret_cast<const float*>(left.constData()),
            n);
    playbackStreams[CHANNEL_RIGHT]->write(nextPlaybackTime,
            reinterpret_cast<const float*>(right.constData()),
            n);

    // Finished with interval?
    if (n == remaining || (n == 0 && interval->appendingFinished())) {
        intervals.removeFirst();
        nextPlaybackTime = session->nextIntervalTime();
        return false;
    }

    nextPlaybackTime += n;
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

void RemoteChannel::enqueueRemoteInterval(SharedRemoteInterval remoteInterval)
{
    // TODO handle silent intervals
    // Start playing the first interval after the current interval
    if (intervals.isEmpty()) {
        nextPlaybackTime = appView->qmlGlobals()->session()->nextIntervalTime();
    }

    intervals.append(remoteInterval);
}
