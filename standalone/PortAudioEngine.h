// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <QObject>
#include <QString>
#include <portaudio.h>
#include "audio/AudioStream.h" // for types and constants

/* How to route an input/output channel */
enum class ChannelRoute {
    OFF,   /* disable channel */
    LEFT,  /* stereo left */
    RIGHT, /* stereo right */
    BOTH,  /* mono mix */
};

class PortAudioEngine : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool running READ running NOTIFY runningChanged)

    Q_PROPERTY(QString hostApi READ hostApi WRITE setHostApi)
    Q_PROPERTY(QStringList availableHostApis READ availableHostApis CONSTANT)

    Q_PROPERTY(QString inputDevice READ inputDevice WRITE setInputDevice)
    Q_PROPERTY(QStringList availableInputDevices READ availableInputDevices NOTIFY availableInputDevicesChanged)
    Q_PROPERTY(QList<ChannelRoute> inputRouting READ inputRouting WRITE setInputRouting)

    Q_PROPERTY(QString outputDevice READ outputDevice WRITE setOutputDevice)
    Q_PROPERTY(QStringList availableOutputDevices READ availableOutputDevices NOTIFY availableOutputDevicesChanged)
    Q_PROPERTY(QList<ChannelRoute> outputRouting READ outputRouting WRITE setOutputRouting)

    Q_PROPERTY(int sampleRate READ sampleRate WRITE setSampleRate)
    Q_PROPERTY(int bufferSize READ bufferSize WRITE setBufferSize)

public:
    // Real-time audio processing callback
    typedef void ProcessFn(float *inOutSamples[CHANNELS_STEREO],
                           size_t nsamples,
                           SampleTime now);

    PortAudioEngine(QObject *parent = nullptr);
    ~PortAudioEngine();

    // Call before start(). It's a separate function so the QML GUI can call
    // start while the C++ code sets the process function.
    void setProcessFn(std::function<ProcessFn> processFn_);

    bool running() const { return stream; }
    const QString &hostApi() const { return hostApi_; }
    const QString &inputDevice() const { return inputDevice_; }
    const QList<ChannelRoute> &inputRouting() const { return inputRouting_; }
    const QString &outputDevice() const { return outputDevice_; }
    const QList<ChannelRoute> &outputRouting() const { return outputRouting_; }
    int sampleRate() const { return sampleRate_; }
    int bufferSize() const { return bufferSize_; }

    void setHostApi(const QString &name);
    void setInputDevice(const QString &name);
    void setInputRouting(const QList<ChannelRoute> &routing);
    void setOutputDevice(const QString &name);
    void setOutputRouting(const QList<ChannelRoute> &routing);
    void setSampleRate(int sampleRate);
    void setBufferSize(int bufferSize);

    /* For enumerating available devices */
    QStringList availableHostApis() const;
    QStringList availableInputDevices() const;
    QStringList availableOutputDevices() const;
    void logDeviceInfo() const;

    Q_INVOKABLE bool start();
    Q_INVOKABLE void stop();

signals:
    // When hostApi changes the available input/output devices also change
    void availableInputDevicesChanged();
    void availableOutputDevicesChanged();

    // Emitted when audio starts/stops
    void runningChanged();

    // Emitted when the engine stops due to an error
    void stoppedUnexpectedly();

private:
    std::function<ProcessFn> processFn;
    PaStream *stream;
    SampleTime now;
    std::vector<float> sampleBuf[CHANNELS_STEREO];
    QString hostApi_;
    QString inputDevice_;
    QList<ChannelRoute> inputRouting_;
    QString outputDevice_;
    QList<ChannelRoute> outputRouting_;
    int sampleRate_; // in Hz
    int bufferSize_; // in samples
    bool stopping; // are we expecting streamFinishedCallback()?

    QStringList availableDevices(bool input, bool output) const;
    void resetChannelRouting(const QString &deviceName,
                             bool isInput,
                             QList<ChannelRoute> *routing);
    bool fillStreamParameters(PaStreamParameters *inputParams,
                              PaStreamParameters *outputParams);
    void mixInputBuffers(const float **input, size_t nsamples);
    void mixOutputBuffers(float **output, size_t nsamples);
    void process(const float **input, float **output, size_t nsamples);
    static void streamFinishedCallback(void *userData);
    static int streamCallback(const void *input_,
                              void *output_,
                              unsigned long frameCount,
                              const PaStreamCallbackTimeInfo* timeInfo,
                              PaStreamCallbackFlags statusFlags,
                              void *userData);

private slots:
    void streamFinished();
};
