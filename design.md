# Design

Wahjam2 is an online jamming client that is designed to support multiple
formats including a standalone application and plugins (VST, AudioUnit).

## Multi-instance architecture
There are very few global variables because plugins can be instantiated
multiple times and the globals may be shared between instances. Therefore
global variables can only be used for true singletons that are not
per-instance. The `AppView` class represents one instance of the client and
most objects are reachable from it.

## QML user interface
The user interface is written in QML and JavaScript. The jam session logic,
networking, and audio code is mostly written in C++. The QML environment
interacts with the C++ objects through `QmlGlobals`, which provides a
JavaScript API and is owned by `AppView`. QML/JavaScript code accesses
`QmlGlobals` by importing `org.wahjam.client` and its `Client` object (the
JavaScript name for `QmlGlobals`).

The QML bindings for C++ QObjects expose Q\_PROPERTYs, methods that have
been marked `Q\_INVOKABLE`, and Qt signals.

## Real-time audio engine
Audio processing has real-time constraints, meaning that audio samples must be
processed within a certain amount of time to prevent glitches, pops, and other
undesirable sounds. This is achieved by separating the audio processing code
from the rest of the application. The audio processing code is written to meet
real-time constraints, which means it does not block, invoke library functions,
take locks, allocate memory, or anything else that might take a long time.

The audio engine plays back and records `AudioStream` objects. Application
threads append audio samples to an `AudioStream`, which has a ring buffer to
decouple the real-time audio thread from application threads that are
producing/consuming audio data.

A consequence of this design is that the application does not process audio
inline. Instead it can only buffer up audio data to be played back in the
future or consume audio data that was recorded in the past. This is achieved by
calling a function from a timer to fill playback buffers and drain capture
buffers.

## Interval time synchronization
It can be challenging to write code that pre-fills buffers with audio data to
be played in the future. If an event occurs that affects the future then
buffered audio data may no longer be appropriate. An example is when the tempo
is changed but the metronome has already buffered audio data for the near
future.

Luckily not many events cause a near-term change. Typically they affect 100s of
milliseconds into the future and the `AudioStream` will not buffer too far. A
small number of settings are implemented in real-time in `AudioStream` so they
do not suffer any delay when changed, such as whether to monitor the stream and
the stereo panning value.

When a jam session is in progress the sample time of the next interval can be
determined with `JamSession::nextIntervalTime()`. The duration of an interval
can be determined by `JamSession::remainingIntervalTime(pos)` where `pos` is
a sample time within the interval being queried. This makes it possible to
synchronize audio to the jam session interval.
