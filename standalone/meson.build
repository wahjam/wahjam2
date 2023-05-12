# SPDX-License-Identifier: Apache-2.0
moc_headers = files(
  'PortAudioEngine.h'
)

sources = files(
  'main.cpp',
  'PortAudioEngine.cpp'
)

executable(appname,
           sources,
	   qt6.preprocess(moc_headers : moc_headers),
	   resources,
	   dependencies : [
	     dependencies,
	     dependency('portaudio-2.0'),
	   ],
	   include_directories : inc,
	   link_with : [libaudio, libcore],
       install : true,
       win_subsystem : 'windows')
