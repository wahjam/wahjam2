// SPDX-License-Identifier: Apache-2.0
#include "config.h"
#ifdef HAVE_PA_JACK_H
#include <pa_jack.h>
#endif
#include "PortAudioEngine.h"

PortAudioEngine::PortAudioEngine(QObject *parent)
    : QObject{parent}, processFn{nullptr}, stream{nullptr}, now{0},
      sampleRate_{44100}, bufferSize_{512}
{
#ifdef HAVE_PA_JACK_H
    // Make it easy for users to identify the application's JACK ports
    PaJack_SetClientName(APPNAME);
#endif

    PaError err = Pa_Initialize();
    if (err != paNoError) {
        qFatal("Pa_Initialize failed: %s", Pa_GetErrorText(err));
    }
}

PortAudioEngine::~PortAudioEngine()
{
    stop();
    Pa_Terminate();
}

void PortAudioEngine::setProcessFn(std::function<ProcessFn> processFn_)
{
    if (stream != nullptr) {
        // In theory it's possible, but memory ordering and thread safety needs
        // to be checked. We won't need it, so forbid it.
        qFatal("Cannot set processFn() after starting stream");
    }

    processFn = processFn_;
}

QStringList PortAudioEngine::availableHostApis() const
{
    const PaHostApiInfo *info;
    QStringList names;

    for (PaHostApiIndex i = 0; (info = Pa_GetHostApiInfo(i)) != nullptr; i++) {
        names.append(info->name);
    }
    return names;
}

// Look up the PortAudio host API index (not the same as the type ID)
static PaHostApiIndex findHostApiIndex(const QString &name)
{
    const PaHostApiInfo *info;

    for (PaHostApiIndex i = 0;
         (info = Pa_GetHostApiInfo(i)) != nullptr;
         i++) {
        if (name == info->name) {
            return i;
        }
    }
    return paHostApiNotFound;
}

// Return a list of devices that support input and/or output
QStringList PortAudioEngine::availableDevices(bool input, bool output) const
{
    auto apiIndex = findHostApiIndex(hostApi_);
    if (apiIndex < 0) {
        return {};
    }

    QStringList names;
    for (PaDeviceIndex i = 0; i < Pa_GetDeviceCount(); i++) {
        auto deviceInfo = Pa_GetDeviceInfo(i);
        if (deviceInfo == nullptr ||
            deviceInfo->hostApi != apiIndex ||
            (input && deviceInfo->maxInputChannels == 0) ||
            (output && deviceInfo->maxOutputChannels == 0)) {
            continue;
        }

        names.append(deviceInfo->name);
    }
    return names;
}

QStringList PortAudioEngine::availableInputDevices() const
{
    return availableDevices(true, false);
}

QStringList PortAudioEngine::availableOutputDevices() const
{
    return availableDevices(false, true);
}

void PortAudioEngine::logDeviceInfo() const
{
    qDebug("%s", Pa_GetVersionText());

    const PaHostApiInfo *apiInfo;
    for (PaHostApiIndex api = 0;
         (apiInfo = Pa_GetHostApiInfo(api)) != nullptr;
         api++) {
        int device;
        for (device = 0; device < apiInfo->deviceCount; device++) {
            PaDeviceIndex devIdx = Pa_HostApiDeviceIndexToDeviceIndex(api, device);
            if (devIdx < 0) {
                qDebug("[%s %02d] Error: %s",
                       apiInfo->name,
                       device,
                       Pa_GetErrorText(devIdx));
                continue;
            }

            const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(devIdx);
            if (!deviceInfo) {
                qDebug("[%s %02d] Invalid device index",
                       apiInfo->name, device);
                continue;
            }

            qDebug("[%s %02d] \"%s\" (%d)%s%s",
                   apiInfo->name, device, deviceInfo->name, devIdx,
                   apiInfo->defaultInputDevice == devIdx ? " [Default Input]" : "",
                   apiInfo->defaultOutputDevice == devIdx ? " [Default Output]" : "");
            qDebug("     Channels: %d in, %d out",
                    deviceInfo->maxInputChannels,
                    deviceInfo->maxOutputChannels);
            qDebug("     Default sample rate: %g Hz",
                    deviceInfo->defaultSampleRate);
            qDebug("     Input latency: %g low, %g high",
                    deviceInfo->defaultLowInputLatency,
                    deviceInfo->defaultHighInputLatency);
            qDebug("     Output latency: %g low, %g high",
                    deviceInfo->defaultLowOutputLatency,
                    deviceInfo->defaultHighOutputLatency);
        }
    }
}

void setDefaultInputRouting(int numChannels, QList<ChannelRoute> *routing)
{
    // There is no way of knowing whether inputs are mono or left/right
    for (int i = 0; i < numChannels; i++) {
        routing->append(ChannelRoute::BOTH);
    }
}

void setDefaultOutputRouting(int numChannels, QList<ChannelRoute> *routing)
{
    // Detect mono output
    if (numChannels == 1) {
        routing->append(ChannelRoute::BOTH);
        return;
    }

    // Multiple output are usually stereo pairs
    for (int i = 0; i < numChannels; i++) {
        routing->append((i % 2) ? ChannelRoute::RIGHT : ChannelRoute::LEFT);
    }
}

// Try to initialize a sane channel routing configuration
void PortAudioEngine::resetChannelRouting(const QString &deviceName,
                                          bool isInput,
                                          QList<ChannelRoute> *routing)
{
    routing->clear();

    auto apiIndex = findHostApiIndex(hostApi_);
    if (apiIndex < 0) {
        return;
    }

    for (PaDeviceIndex i = 0; i < Pa_GetDeviceCount(); i++) {
        auto info = Pa_GetDeviceInfo(i);
        if (info == nullptr ||
            info->hostApi != apiIndex ||
            deviceName != info->name) {
            continue;
        }

        if (isInput && info->maxInputChannels > 0) {
            setDefaultInputRouting(info->maxInputChannels, routing);
        }
        if (!isInput && info->maxOutputChannels > 0) {
            setDefaultOutputRouting(info->maxOutputChannels, routing);
        }
    }
}

void PortAudioEngine::setHostApi(const QString &name)
{
    if (stream != nullptr) {
        qDebug("Cannot change properties while audio stream is running");
        return;
    }

    hostApi_ = name;
    resetChannelRouting(inputDevice_, true, &inputRouting_);
    resetChannelRouting(outputDevice_, false, &outputRouting_);

    emit availableInputDevicesChanged();
    emit availableOutputDevicesChanged();
}

void PortAudioEngine::setInputDevice(const QString &name)
{
    if (stream != nullptr) {
        qDebug("Cannot change properties while audio stream is running");
        return;
    }

    inputDevice_ = name;
    resetChannelRouting(inputDevice_, true, &inputRouting_);
}

void PortAudioEngine::setInputRouting(const QList<ChannelRoute> &routing)
{
    if (stream != nullptr) {
        qDebug("Cannot change properties while audio stream is running");
        return;
    }
    if (routing.size() != inputRouting_.size()) {
        qDebug("Got input routing with size %d, expected %d",
               routing.size(),
               inputRouting_.size());
        return;
    }

    inputRouting_ = routing;
}

void PortAudioEngine::setOutputDevice(const QString &name)
{
    if (stream != nullptr) {
        qDebug("Cannot change properties while audio stream is running");
        return;
    }
    outputDevice_ = name;
    resetChannelRouting(outputDevice_, false, &outputRouting_);
}

void PortAudioEngine::setOutputRouting(const QList<ChannelRoute> &routing)
{
    if (stream != nullptr) {
        qDebug("Cannot change properties while audio stream is running");
        return;
    }
    if (routing.size() != outputRouting_.size()) {
        qDebug("Got output routing with size %d, expected %d",
               routing.size(),
               outputRouting_.size());
        return;
    }

    outputRouting_ = routing;
}

void PortAudioEngine::setSampleRate(int sampleRate)
{
    if (stream != nullptr) {
        qDebug("Cannot change properties while audio stream is running");
        return;
    }

    sampleRate_ = sampleRate;
}

void PortAudioEngine::setBufferSize(int bufferSize)
{
    if (stream != nullptr) {
        qDebug("Cannot change properties while audio stream is running");
        return;
    }

    bufferSize_ = bufferSize;
}

static void zeroSamples(float *output, size_t nsamples)
{
    for (size_t i = 0; i < nsamples; i++) {
        output[i] = 0;
    }
}

static const float UNITY_GAIN = 1.f;
static const float PAN_LAW = 0.5f; // -3 dB

// Mix samples into a buffer that may be uninitialized
static void mixSamples(const float *input, float *output, size_t nsamples,
                       float gain, bool inited)
{
    if (inited) {
        for (size_t i = 0; i < nsamples; i++) {
            output[i] += input[i] * gain;
        }
    } else {
        for (size_t i = 0; i < nsamples; i++) {
            output[i] = input[i] * gain;
        }
    }
}

// Mix input channels into the sample buffers
void PortAudioEngine::mixInputBuffers(const float **input, size_t nsamples)
{
    bool inited[CHANNELS_STEREO] = {false, false};

    if (input) {
        for (auto i = 0; i < inputRouting_.size(); i++) {
            switch (inputRouting_[i]) {
            case ChannelRoute::OFF:
                break; // do nothing
            case ChannelRoute::LEFT:
                mixSamples(input[i], sampleBuf[CHANNEL_LEFT].data(), nsamples,
                           UNITY_GAIN, inited[CHANNEL_LEFT]);
                inited[CHANNEL_LEFT] = true;
                break;
            case ChannelRoute::RIGHT:
                mixSamples(input[i], sampleBuf[CHANNEL_RIGHT].data(), nsamples,
                           UNITY_GAIN, inited[CHANNEL_RIGHT]);
                inited[CHANNEL_RIGHT] = true;
                break;
            case ChannelRoute::BOTH:
                mixSamples(input[i], sampleBuf[CHANNEL_LEFT].data(), nsamples,
                           PAN_LAW, inited[CHANNEL_LEFT]);
                mixSamples(input[i], sampleBuf[CHANNEL_RIGHT].data(), nsamples,
                           PAN_LAW, inited[CHANNEL_RIGHT]);
                inited[CHANNEL_LEFT] = true;
                inited[CHANNEL_RIGHT] = true;
                break;
            }
        }
    }

    if (!inited[CHANNEL_LEFT]) {
        zeroSamples(sampleBuf[CHANNEL_LEFT].data(), nsamples);
    }
    if (!inited[CHANNEL_RIGHT]) {
        zeroSamples(sampleBuf[CHANNEL_RIGHT].data(), nsamples);
    }
}

// Mix sample buffers into the output channels
void PortAudioEngine::mixOutputBuffers(float **output, size_t nsamples)
{
    if (!output) {
        return;
    }

    for (auto i = 0; i < outputRouting_.size(); i++) {
        switch (outputRouting_[i]) {
        case ChannelRoute::OFF:
            zeroSamples(output[i], nsamples);
            break;
        case ChannelRoute::LEFT:
            mixSamples(sampleBuf[CHANNEL_LEFT].data(), output[i],
                       nsamples, UNITY_GAIN, false);
            break;
        case ChannelRoute::RIGHT:
            mixSamples(sampleBuf[CHANNEL_RIGHT].data(), output[i],
                       nsamples, UNITY_GAIN, false);
            break;
        case ChannelRoute::BOTH:
            mixSamples(sampleBuf[CHANNEL_LEFT].data(), output[i],
                       nsamples, PAN_LAW, false);
            mixSamples(sampleBuf[CHANNEL_RIGHT].data(), output[i],
                       nsamples, PAN_LAW, true);
            break;
        }
    }
}

void PortAudioEngine::process(const float **input,
                              float **output,
                              size_t nsamples)
{
    // The sample buffers should be more than large enough, ignore this case
    if (nsamples > sampleBuf[CHANNEL_LEFT].size()) {
        for (auto i = 0; i < outputRouting_.size(); i++) {
            zeroSamples(output[i], nsamples);
        }
        return;
    }

    mixInputBuffers(input, nsamples);

    float *inOutSamples[] = {
        sampleBuf[CHANNEL_LEFT].data(),
        sampleBuf[CHANNEL_RIGHT].data(),
    };
    processFn(inOutSamples, nsamples, now);

    mixOutputBuffers(output, nsamples);
    now += nsamples;
}

int PortAudioEngine::streamCallback(const void *input_,
                                    void *output_,
                                    unsigned long frameCount,
                                    const PaStreamCallbackTimeInfo* timeInfo,
                                    PaStreamCallbackFlags statusFlags,
                                    void *userData)
{
    auto input = (const float **)input_;
    auto output = reinterpret_cast<float **>(output_);
    auto engine = reinterpret_cast<PortAudioEngine *>(userData);

    Q_UNUSED(timeInfo); // TODO use timestamp instead of frame time
    Q_UNUSED(statusFlags); // TODO handle over/underflows?

    engine->process(input, output, frameCount);
    return paContinue;
}

// Called when the stream stopped unexpectedly with an error
void PortAudioEngine::streamFinished()
{
    auto info = Pa_GetLastHostErrorInfo();
    qCritical("PortAudio stream %p stopped unexpectedly with error code %ld (\"%s\")",
              stream,
              info->errorCode,
              info->errorText);
    stop();

    emit stoppedUnexpectedly();
}

// PortAudio may call this from an arbitrary thread
void PortAudioEngine::streamFinishedCallback(void *userData)
{
    auto engine = reinterpret_cast<PortAudioEngine *>(userData);

    // Ignore if stopping is expected
    if (engine->stopping) {
        return;
    }

    // Perform further processing in engine's event loop thread
    QMetaObject::invokeMethod(engine, "streamFinished", Qt::QueuedConnection);
}

// Returns a string representation of a channel routing configuration
QString formatRouting(const QList<ChannelRoute> &routing)
{
    QString output;
    for (auto route : routing) {
        switch (route) {
            case ChannelRoute::OFF:
                output += '-';
                break;
            case ChannelRoute::LEFT:
                output += 'L';
                break;
            case ChannelRoute::RIGHT:
                output += 'R';
                break;
            case ChannelRoute::BOTH:
                output += 'B';
                break;
        }
    }
    return output;
}

bool PortAudioEngine::fillStreamParameters(PaStreamParameters *inputParams,
                                           PaStreamParameters *outputParams)
{
    auto apiIndex = findHostApiIndex(hostApi_);
    if (apiIndex < 0) {
        qCritical("%s hostApi \"%s\" not found",
                  __func__, hostApi_.toLatin1().constData());
        return false;
    }

    if (inputDevice_.isEmpty() && outputDevice_.isEmpty()) {
        qCritical("No input or output device given");
        return false;
    }
    if (sampleRate_ <= 0) {
        qCritical("Invalid sample rate %d", sampleRate_);
        return false;
    }
    if (bufferSize_ <= 0) {
        qCritical("Invalid buffer size %d", bufferSize_);
        return false;
    }

    PaTime latency = bufferSize_ / (PaTime)sampleRate_;
    bool foundInput = false;
    bool foundOutput = false;

    for (PaDeviceIndex i = 0; i < Pa_GetDeviceCount(); i++) {
        auto info = Pa_GetDeviceInfo(i);
        if (info == nullptr || info->hostApi != apiIndex) {
            continue;
        }

        PaStreamParameters params = {
            i,
            0, // filled in below
            paFloat32 | paNonInterleaved,
            0, // filled in below
            nullptr,
        };

        if (inputDevice_ == info->name && inputParams != nullptr) {
            params.channelCount = info->maxInputChannels;
            params.suggestedLatency =
                qMax(latency, info->defaultLowInputLatency);

            if (inputRouting_.size() != params.channelCount) {
                qCritical("Expected %d input channel routes, got %d",
                          params.channelCount,
                          inputRouting_.size());
                return false;
            }

            *inputParams = params;
            foundInput = true;
        }
        if (outputDevice_ == info->name && outputParams != nullptr) {
            params.channelCount = info->maxOutputChannels;
            params.suggestedLatency =
                qMax(latency, info->defaultLowOutputLatency);

            if (outputRouting_.size() != params.channelCount) {
                qCritical("Expected %d output channel routes, got %d",
                          params.channelCount,
                          outputRouting_.size());
                return false;
            }

            *outputParams = params;
            foundOutput = true;
        }
    }

    if (!foundInput && !inputDevice_.isEmpty()) {
        qCritical("Could not find input device \"%s\"",
                  inputDevice_.toLatin1().constData());
        return false;
    }
    if (!foundOutput && !outputDevice_.isEmpty()) {
        qCritical("Could not find input device \"%s\"",
                  outputDevice_.toLatin1().constData());
        return false;
    }
    if (foundInput) {
        qDebug("Input device: [%s] \"%s\" (%d)",
               hostApi_.toLatin1().constData(),
               inputDevice_.toLatin1().constData(),
               inputParams->device);
        qDebug("Channels: %d [%s]",
               inputParams->channelCount,
               formatRouting(inputRouting_).toLatin1().constData());
        qDebug("Latency: %g secs", inputParams->suggestedLatency);
    }
    if (foundOutput) {
        qDebug("Output device: [%s] \"%s\" (%d)",
               hostApi_.toLatin1().constData(),
               outputDevice_.toLatin1().constData(),
               outputParams->device);
        qDebug("Channels: %d [%s]",
               outputParams->channelCount,
               formatRouting(outputRouting_).toLatin1().constData());
        qDebug("Latency: %g secs", outputParams->suggestedLatency);
    }
    return true;
}

bool PortAudioEngine::start()
{
    if (stream != nullptr) {
        return false;
    }
    if (processFn == nullptr) {
        return false;
    }

    PaStreamParameters inputParams_, outputParams_;
    PaStreamParameters *inputParams =
        inputDevice_.isEmpty() ? nullptr : &inputParams_;
    PaStreamParameters *outputParams =
        outputDevice_.isEmpty() ? nullptr : &outputParams_;
    if (!fillStreamParameters(inputParams, outputParams)) {
        return false;
    }

    PaError err;
    err = Pa_OpenStream(&stream,
                        inputParams,
                        outputParams,
                        sampleRate_,
                        paFramesPerBufferUnspecified,
                        paNoFlag,
                        &PortAudioEngine::streamCallback,
                        this);
    if (err != paNoError) {
        stream = nullptr;
        qCritical("Pa_OpenStream failed: %s", Pa_GetErrorText(err));

        auto info = Pa_GetLastHostErrorInfo();
        qCritical("Host error code %ld (\"%s\")",
                  info->errorCode,
                  info->errorText);

        return false;
    }

    auto streamInfo = Pa_GetStreamInfo(stream);
    if (streamInfo) {
        qDebug("PortAudio stream %p opened with sample rate %g Hz, input latency %g secs, output latency %g secs",
               stream,
               streamInfo->sampleRate,
               streamInfo->inputLatency,
               streamInfo->outputLatency);

        // Update our settings
        sampleRate_ = streamInfo->sampleRate;
        int newBufferSize = qMax(sampleRate_ * streamInfo->inputLatency,
                                 sampleRate_ * streamInfo->outputLatency);
        bufferSize_ = qMax(bufferSize_, newBufferSize);
    }

    err = Pa_SetStreamFinishedCallback(stream, streamFinishedCallback);
    if (err != paNoError) {
        qCritical("Pa_SetStreamFinishedCallback failed: %s",
                  Pa_GetErrorText(err));
        Pa_CloseStream(stream);
        stream = nullptr;
        return false;
    }

    emit runningChanged();

    // Allocate buffers with plenty of space
    sampleBuf[CHANNEL_LEFT].resize(bufferSize_ * 2);
    sampleBuf[CHANNEL_RIGHT].resize(bufferSize_ * 2);

    // Restart sample clock
    now = 0;

    err = Pa_StartStream(stream);
    if (err != paNoError) {
        qCritical("Pa_StartStream failed: %s", Pa_GetErrorText(err));
        stop();
        return false;
    }

    return true;
}

void PortAudioEngine::stop()
{
    if (stream == nullptr) {
        return;
    }

    stopping = true;

    PaError err = Pa_CloseStream(stream);
    if (err == paNoError) {
        qDebug("PortAudio stream %p closed successfully", stream);
    } else {
        qCritical("Pa_CloseStream stream %p failed: %s",
                  stream,
                  Pa_GetErrorText(err));
    }

    stream = nullptr;
    stopping = false;

    emit runningChanged();
}
