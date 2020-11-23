# SPDX-License-Identifier: Apache-2.0
moc_headers = files(
  'AppView.h',
  'JamApiManager.h',
  'JamConnection.h',
  'JamSession.h',
  'Metronome.h',
  'OggVorbisDecoder.h',
  'QmlGlobals.h',
  'RemoteInterval.h',
  'Resampler.h',
  'SessionListModel.h',
)

sources = files(
  'AppView.cpp',
  'global.cpp',
  'JamApiManager.cpp',
  'JamConnection.cpp',
  'JamSession.cpp',
  'Metronome.cpp',
  'OggVorbisDecoder.cpp',
  'QmlGlobals.cpp',
  'RemoteInterval.cpp',
  'Resampler.cpp',
  'SessionListModel.cpp',
)

libcore = static_library('core',
                         sources,
                         qt5.preprocess(moc_headers : moc_headers),
                         dependencies : dependencies,
                         include_directories : inc)
