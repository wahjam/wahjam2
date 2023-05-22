# SPDX-License-Identifier: Apache-2.0
moc_headers = files(
  'VstPlugin.h'
)

sources = files(
  'VstPlugin.cpp'
)

shared_library(appname,
               sources,
               qt6.preprocess(moc_headers : moc_headers),
               resources,
               dependencies : dependencies,
               include_directories : inc,
               link_with : [libaudio, libcore],
               name_prefix : '',
               install : false)
