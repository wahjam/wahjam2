// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <QUuid>
#include "audio/AudioStream.h"
#include "IIntervalTime.h"
#include "OggVorbisEncoder.h"

/*
 * An audio channel that processes data from a local sound source. Handles
 * uploading intervals as well as pan and monitoring.
 */
class LocalChannel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(bool send READ send WRITE setSend)
    Q_PROPERTY(float peakVolume READ peakVolume NOTIFY peakVolumeChanged)
    // TODO pan, gain

public:
    // Does not take ownership of captureLeft and captureRight
    LocalChannel(const QString &name,
                 int channelIdx,
                 AudioStream *captureLeft,
                 AudioStream *captureRight,
                 int sampleRate,
                 IIntervalTime *intervalTime,
                 QObject *parent = nullptr);

    QString name() const;
    void setName(const QString &name);
    bool send() const;
    void setSend(bool enable);
    float peakVolume() const;

    // Begin uploading data from the capture streams
    void start();

public slots:
    void processAudioStreams();

signals:
    // Emitted when there is compressed audio data available for uploading.
    // This signal is emitted at least once per interval. The first time this
    // signal is emitted each interval 'first' is true. The last time the
    // signal is emitted each interval 'last' is true. 'data' may be empty.
    void uploadData(int channelIdx, const QUuid &uuid, const QByteArray &data,
                    bool first, bool last);

    void nameChanged();
    void peakVolumeChanged();

private:
    IIntervalTime *intervalTime;
    AudioStream *captureStreams[CHANNELS_STEREO];
    QString name_;
    int channelIdx;
    bool send_;
    bool nextSend;
    bool firstUploadData;
    QUuid guid;
    OggVorbisEncoder encoder;
    SampleTime nextCaptureTime;
    size_t remainingIntervalTime;
    bool nextCaptureTimeValid;
};
