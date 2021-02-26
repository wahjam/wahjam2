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
    for (auto channel : qAsConst(channels)) {
        delete channel;
    }
    channels.clear();
}

QString RemoteUser::username() const
{
    return username_;
}

int RemoteUser::numActiveChannels() const
{
    int num = 0;
    for (auto channel : qAsConst(channels)) {
        if (channel) {
            num++;
        }
    }
    return num;
}

void RemoteUser::setChannelInfo(int channelIndex, const QString &channelName,
                                bool active)
{
    RemoteChannel *channel = channels.value(channelIndex, nullptr);

    // TODO emit notifications for UI?
    if (channel && !active) {
        channels.remove(channelIndex);
        delete channel;
    } else if (channel && active) {
        channel->setName(channelName);
    } else if (!channel && active) {
        channel = new RemoteChannel{channelName, appView};
        channels.insert(channelIndex, channel);
        connect(appView, &AppView::processAudioStreams,
                channel, &RemoteChannel::processAudioStreams);
    }
}

bool RemoteUser::enqueueRemoteInterval(int channelIndex,
                                       std::shared_ptr<RemoteInterval> remoteInterval)
{
    RemoteChannel *channel = channels.value(channelIndex, nullptr);
    if (!channel) {
        return false;
    }

    channel->enqueueRemoteInterval(remoteInterval);
    return true;
}
