// SPDX-License-Identifier: Apache-2.0
#include <QSettings>
#include "LocalChannel.h"

static bool dumpLocalChannelsEnabled()
{
    QSettings settings;
    return settings.value("debug/dumpLocalChannels").toBool();
}

LocalChannel::LocalChannel(const QString &name,
                           int channelIdx_,
                           AudioStream *captureLeft,
                           AudioStream *captureRight,
                           AudioProcessor *processor_,
                           IIntervalTime *intervalTime_,
                           QObject *parent)
    : QObject(parent),
      processor{processor_},
      intervalTime{intervalTime_},
      captureStreams{captureLeft, captureRight},
      name_{name},
      channelIdx{channelIdx_},
      send_{false},
      nextSend{true},
      firstUploadData{true},
      started{false},
      encoder{1, processor_->getSampleRate()},
      nextCaptureTime{0},
      nextCaptureTimeValid{false},
      dumpFileEnabled{dumpLocalChannelsEnabled()},
      dumpFileNum{0}
{
}

QString LocalChannel::name() const
{
    return name_;
}

void LocalChannel::setName(const QString &name)
{
    name_ = name;
    emit nameChanged();
}

bool LocalChannel::send() const
{
    return nextSend;
}

void LocalChannel::setSend(bool enable)
{
    if (nextSend == enable) {
        return;
    }
    nextSend = enable;
    emit sendChanged();
}

float LocalChannel::peakVolume() const
{
    // TODO make peak volume monitoring stereo?
    return (captureStreams[CHANNEL_LEFT]->getPeakVolume() +
            captureStreams[CHANNEL_RIGHT]->getPeakVolume()) / 2.f;
}

void LocalChannel::start()
{
    started = true;
    nextCaptureTimeValid = false;
    // TODO sample-accurate way to sync up to start of interval
}

void LocalChannel::stop()
{
    started = false;
}

void LocalChannel::processAudioStreams()
{
    // Periodically emit signal since peak volume is always changing
    emit peakVolumeChanged();

    if (captureStreams[CHANNEL_LEFT]->checkResetAndClear() ||
        captureStreams[CHANNEL_RIGHT]->checkResetAndClear()) {
        encoder.reset(processor->getSampleRate());
    }

    if (!started) {
        captureStreams[CHANNEL_LEFT]->readDiscardAll();
        captureStreams[CHANNEL_RIGHT]->readDiscardAll();
        return;
    }

    // Synchronize time to the capture stream
    if (!nextCaptureTimeValid) {
        if (!captureStreams[CHANNEL_LEFT]->peekReadSampleTime(&nextCaptureTime)) {
            return;
        }

        // Discard any samples from before the current interval
        SampleTime start = intervalTime->currentIntervalTime();
        if (nextCaptureTime < start) {
            size_t numDiscard = start - nextCaptureTime;
            captureStreams[CHANNEL_LEFT]->readDiscard(nextCaptureTime, numDiscard);
            captureStreams[CHANNEL_RIGHT]->readDiscard(nextCaptureTime, numDiscard);
            nextCaptureTime = start;
        }

        // Don't start capturing if we haven't reached the start of the interval yet
        if (!captureStreams[CHANNEL_LEFT]->peekReadSampleTime(&nextCaptureTime)) {
            return;
        }

        remainingIntervalTime = intervalTime->remainingIntervalTime(nextCaptureTime);
        firstUploadData = true;
        nextCaptureTimeValid = true;
    }

    size_t readable =
        qMin(captureStreams[CHANNEL_LEFT]->numSamplesReadable(),
             captureStreams[CHANNEL_RIGHT]->numSamplesReadable());

    // Mix down to mono for now. Don't use AudioStream::readMixStereo() because
    // that relies on AudioStream::pan. Leave AudioStream::pan alone for now.
    // Stereo support will be added later and then panning can be done properly.
    auto left = std::vector<float>(readable);
    auto right = std::vector<float>(readable);
    auto samples = std::vector<float>(readable, 0.f);
    size_t n;
    n = captureStreams[CHANNEL_LEFT]->read(nextCaptureTime, left.data(), readable);
    readable = qMin(n, readable); // in case data was discarded
    n = captureStreams[CHANNEL_RIGHT]->read(nextCaptureTime, right.data(), readable);
    assert(n == readable);
    float pan = 0.5f; // linear stereo pan
    mixSamples(left.data(), samples.data(), readable, pan);
    mixSamples(right.data(), samples.data(), readable, pan);

    for (size_t i = 0; i < readable; i += n) {
        n = qMin(readable - i, remainingIntervalTime); // only process up to next interval
        remainingIntervalTime -= n;
        nextCaptureTime += n;

        if (send_) {
            QByteArray data = encoder.encode(samples.data() + i, nullptr, n);
            if (remainingIntervalTime == 0) {
                data.append(encoder.encode(nullptr, nullptr, 0)); // drain encoder
                if (dumpFileEnabled) {
                    dumpFile.write(data);
                }
                emit uploadData(channelIdx, guid, data, firstUploadData, true);
            } else if (data.size() > 0) {
                if (dumpFileEnabled) {
                    if (firstUploadData) {
                        dumpFile.close();
                        dumpFile.setFileName(QString("dump-%1-%2.ogg").arg(name_).arg(dumpFileNum++));
                        if (!dumpFile.open(QIODevice::WriteOnly)) {
                            qWarning("Failed to open dump file \"%s\"",
                                     dumpFile.fileName().toLatin1().constData());
                        }
                    }
                    dumpFile.write(data);
                }

                emit uploadData(channelIdx, guid, data, firstUploadData, false);
                firstUploadData = false;
            }
        }

        if (remainingIntervalTime == 0) {
            send_ = nextSend;
            firstUploadData = true;
            if (send_) {
                guid = QUuid::createUuid(); // random UUID
            } else {
                guid = QUuid(); // null UUID for silence
            }
            encoder.reset();

            // Still emit uploadData once per silent interval
            if (!send_) {
                emit uploadData(channelIdx, guid, QByteArray{}, true, true);
            }

            remainingIntervalTime = intervalTime->remainingIntervalTime(nextCaptureTime);
        }
    }
}
