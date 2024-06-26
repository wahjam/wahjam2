// SPDX-License-Identifier: Apache-2.0
#include "JamSession.h"
#include "QmlGlobals.h"
#include "RemoteChannel.h"

RemoteChannel::RemoteChannel(const QString &name,
                             AppView *appView_,
                             QObject *parent)
    : QObject{parent}, appView{appView_}, name_{name}, nextPlaybackTime{0},
      intervalStartTime{0} {
    AudioProcessor *processor = appView->audioProcessor();

    playbackStreams[CHANNEL_LEFT] = new AudioStream;
    playbackStreams[CHANNEL_RIGHT] = new AudioStream;
    processor->addPlaybackStream(playbackStreams[CHANNEL_LEFT]);
    processor->addPlaybackStream(playbackStreams[CHANNEL_RIGHT]);
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
    return playbackStreams[CHANNEL_LEFT]->monitorEnabled();
}

void RemoteChannel::setMonitorEnabled(bool enable)
{
    playbackStreams[CHANNEL_LEFT]->setMonitorEnabled(enable);
    playbackStreams[CHANNEL_RIGHT]->setMonitorEnabled(enable);
    emit monitorEnabledChanged(enable);
}

bool RemoteChannel::underflow() const
{
    return false; // TODO
}

bool RemoteChannel::remoteSending() const
{
    if (intervals.isEmpty()) {
        return false;
    }
    return !intervals.last()->isSilence();
}

float RemoteChannel::peakVolume() const
{
    // TODO make peak volume monitoring stereo?
    return (playbackStreams[CHANNEL_LEFT]->getPeakVolume() +
            playbackStreams[CHANNEL_RIGHT]->getPeakVolume()) / 2.f;
}

float RemoteChannel::gain() const
{
    return playbackStreams[CHANNEL_LEFT]->getGain();
}

void RemoteChannel::setGain(float gain_)
{
    if (gain_ == gain()) {
        return;
    }

    playbackStreams[CHANNEL_LEFT]->setGain(gain_);
    playbackStreams[CHANNEL_RIGHT]->setGain(gain_);
    emit gainChanged();
}

// Play nsamples of silence
void RemoteChannel::fillWithSilence(size_t nsamples)
{
    std::vector<float> silence(nsamples, 0.f);
    playbackStreams[CHANNEL_LEFT]->write(nextPlaybackTime,
                                         silence.data(),
                                         nsamples);
    playbackStreams[CHANNEL_RIGHT]->write(nextPlaybackTime,
                                          silence.data(),
                                          nsamples);
}

// Play nsamples from current interval and return actual sample count played
size_t RemoteChannel::fillFromInterval(size_t nsamples)
{
    auto interval = intervals.first();
    interval->setSampleRate(appView->audioProcessor()->getSampleRate());

    QByteArray left, right;
    size_t n = interval->decode(&left, &right, nsamples);

    playbackStreams[CHANNEL_LEFT]->write(nextPlaybackTime,
            reinterpret_cast<const float*>(left.constData()),
            n);
    playbackStreams[CHANNEL_RIGHT]->write(nextPlaybackTime,
            reinterpret_cast<const float*>(right.constData()),
            n);

    return n;
}

// Returns true if done, false if we should try again
bool RemoteChannel::fillPlaybackStreams()
{
    JamSession *session = appView->qmlGlobals()->session();
    size_t nwritable =
        qMin(playbackStreams[CHANNEL_LEFT]->numSamplesWritable(),
             playbackStreams[CHANNEL_RIGHT]->numSamplesWritable());
    size_t remaining = nextPlaybackTime < intervalStartTime ?
                       intervalStartTime - nextPlaybackTime :
                       session->remainingIntervalTime(nextPlaybackTime);
    size_t n = qMin(nwritable, remaining);

    if (intervals.isEmpty() || nextPlaybackTime < intervalStartTime){
        fillWithSilence(n);
    } else {
        size_t fill = n;
        n = fillFromInterval(fill);

        bool underflow = n < fill;
        if (underflow) {
            // TODO signal underflow
            intervalStartTime = nextPlaybackTime + remaining;
        }

        bool finishedInterval = n == remaining;
        if (finishedInterval || underflow) {
            intervals.removeFirst();

            if (intervals.isEmpty()) {
                emit remoteSendingChanged(false);
            } else if (intervals.first()->isSilence()) {
                resampler[CHANNEL_LEFT].reset();
                resampler[CHANNEL_RIGHT].reset();
            }
        }
    }

    nextPlaybackTime += n;
    return n == nwritable;
}

void RemoteChannel::processAudioStreams()
{
    bool wasResetLeft = playbackStreams[CHANNEL_LEFT]->checkResetAndClear();
    bool wasResetRight = playbackStreams[CHANNEL_RIGHT]->checkResetAndClear();
    if (wasResetLeft || wasResetRight) {
        nextPlaybackTime = appView->currentSampleTime();
        resampler[CHANNEL_LEFT].reset();
        resampler[CHANNEL_RIGHT].reset();
    }

    while (!fillPlaybackStreams()) {
        // Do nothing
    }

    // Periodically emit signal since peak volume is always changing
    emit peakVolumeChanged();
}

void RemoteChannel::enqueueRemoteInterval(SharedRemoteInterval remoteInterval)
{
    bool oldSilence = true;
    bool newSilence = remoteInterval->isSilence();

    if (intervals.isEmpty()) {
        intervalStartTime = appView->qmlGlobals()->session()->nextIntervalTime();
    } else {
        oldSilence = intervals.last()->isSilence();
    }

    // Resampling is stateful so all intervals share one resampler
    remoteInterval->setResampler(&resampler[CHANNEL_LEFT],
                                 &resampler[CHANNEL_RIGHT]);

    intervals.append(remoteInterval);

    if (oldSilence != newSilence) {
        emit remoteSendingChanged(!newSilence);
    }
}
