# SPDX-License-Identifier: Apache-2.0
sources = files(
  'main.cpp'
)

executable(meson.project_name(),
           sources,
	   qt5.preprocess(moc_headers : moc_headers),
	   resources,
	   dependencies : dependencies,
	   include_directories : inc,
	   link_with : [libaudio, libcore])
