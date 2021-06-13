// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "AppView.h"
#include "RemoteChannel.h"

class RemoteUser : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString username READ username)

public:
    RemoteUser(const QString &username,
               AppView *appView,
               QObject *parent = nullptr);
    ~RemoteUser();

    QString username() const;
    int numActiveChannels() const;
    void setChannelInfo(int channelIndex, const QString &channelName, bool active);

    // Returns true on success, false on failure
    bool enqueueRemoteInterval(int channelIndex,
                               std::shared_ptr<RemoteInterval> remoteInterval);

private:
    AppView *appView;
    QString username_;
    QVector<RemoteChannel*> channels;
};
