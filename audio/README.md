# Audio Engine

The audio engine mixes audio streams and runs in a real-time audio thread.
`AudioProcessor` is the engine class and `AudioStream` objects can be
configured for playback and capture of audio samples.

The real-time audio thread must not block for unbounded periods of time.
Therefore it cannot invoke functions that may block, including taking locks,
allocating memory, and making system calls. Methods that follow this rule are
marked `realtime` and may be called from the real-time audio thread.

Note that the audio engine code does not use Qt in order to have full control
over real-time constraints.

A non-real-time thread sets up `AudioStream` objects and adds them to the
`AudioProcessor`. The non-blocking `AudioStream::read()` and
`AudioStream::write()` methods can be called periodically to transfer audio
samples from/to the real-time audio thread. `AudioStream::tick()` must be
called periodically.
