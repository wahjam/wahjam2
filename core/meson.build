# SPDX-License-Identifier: Apache-2.0
moc_headers = files(
  'JamApiManager.h',
  'JamConnection.h',
  'Metronome.h',
  'SessionListModel.h',
)

sources = files(
  'global.cpp',
  'JamApiManager.cpp',
  'JamConnection.cpp',
  'Metronome.cpp',
  'SessionListModel.cpp',
)

libcore = static_library('core',
                         sources,
			 qt5.preprocess(moc_headers : moc_headers),
			 dependencies : dependencies,
			 include_directories : inc)
