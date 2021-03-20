# SPDX-License-Identifier: Apache-2.0
moc_headers = files(
  'AppView.h',
  'JamApiManager.h',
  'JamConnection.h',
  'JamSession.h',
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

sources = files(
  'AppView.cpp',
  'global.cpp',
  'JamApiManager.cpp',
  'JamConnection.cpp',
  'JamSession.cpp',
  'Metronome.cpp',
  'OggVorbisDecoder.cpp',
  'OggVorbisEncoder.cpp',
  'QmlGlobals.cpp',
  'RemoteChannel.cpp',
  'RemoteInterval.cpp',
  'RemoteUser.cpp',
  'Resampler.cpp',
  'SessionListModel.cpp',
)

libcore = static_library('core',
                         sources,
                         qt5.preprocess(moc_headers : moc_headers),
                         dependencies : dependencies,
                         include_directories : inc)
