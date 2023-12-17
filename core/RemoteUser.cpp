// SPDX-License-Identifier: Apache-2.0
#include "RemoteUser.h"

RemoteUser::RemoteUser(const QString &username,
                       AppView *appView_,
                       QObject *parent)
    : QObject{parent}, appView{appView_}, username_{username}
{
}

RemoteUser::~RemoteUser()
{
    for (auto channel : std::as_const(channels_)) {
        delete channel;
    }
    channels_.clear();
}

QString RemoteUser::username() const
{
    return username_;
}

int RemoteUser::numActiveChannels() const
{
    int num = 0;
    for (auto channel : std::as_const(channels_)) {
        if (channel) {
            num++;
        }
    }
    return num;
}

void RemoteUser::setChannelInfo(int channelIndex, const QString &channelName,
                                bool active)
{
    RemoteChannel *channel = channels_.value(channelIndex, nullptr);

    if (channel && !active) {
        channels_.remove(channelIndex);
        emit channelsChanged();
        delete channel;
    } else if (channel && active) {
        channel->setName(channelName);
    } else if (!channel && active) {
        channel = new RemoteChannel{channelName, appView};
        channels_.insert(channelIndex, channel);
        connect(appView, &AppView::processAudioStreams,
                channel, &RemoteChannel::processAudioStreams);
        emit channelsChanged();
    }
}

const QVector<RemoteChannel*> RemoteUser::channels() const
{
    return channels_;
}

bool RemoteUser::enqueueRemoteInterval(int channelIndex,
                                       std::shared_ptr<RemoteInterval> remoteInterval)
{
    RemoteChannel *channel = channels_.value(channelIndex, nullptr);
    if (!channel) {
        return false;
    }

    channel->enqueueRemoteInterval(remoteInterval);
    return true;
}
