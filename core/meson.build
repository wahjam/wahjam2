# SPDX-License-Identifier: Apache-2.0
moc_headers = files(
  'AppView.h',
  'JamApiManager.h',
  'JamConnection.h',
  'JamSession.h',
  'LocalChannel.h',
  'Metronome.h',
  'OggVorbisDecoder.h',
  'OggVorbisEncoder.h',
  'QmlGlobals.h',
  'RemoteChannel.h',
  'RemoteInterval.h',
  'RemoteUser.h',
  'Resampler.h',
  'SessionListModel.h',
)

sources = [files(
  'AppView.cpp',
  'global.cpp',
  'JamApiManager.cpp',
  'JamConnection.cpp',
  'JamSession.cpp',
  'LocalChannel.cpp',
  'Metronome.cpp',
  'OggVorbisDecoder.cpp',
  'OggVorbisEncoder.cpp',
  'QmlGlobals.cpp',
  'RemoteChannel.cpp',
  'RemoteInterval.cpp',
  'RemoteUser.cpp',
  'Resampler.cpp',
  'SessionListModel.cpp',
)]

if target_machine.system() == 'darwin'
    sources += [files('screensleep_mac.cpp')]
    dependencies += [dependency('appleframeworks', modules: ['Foundation', 'IOKit'])]
elif target_machine.system() == 'windows'
    sources += [files('screensleep_win32.cpp')]
else
    sources += [files('screensleep_stub.cpp')]
endif

libcore = static_library('core',
                         sources,
                         qt6.preprocess(moc_headers : moc_headers),
                         dependencies : dependencies,
                         include_directories : inc)
