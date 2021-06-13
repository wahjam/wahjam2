# SPDX-License-Identifier: Apache-2.0
project('wahjam2', 'cpp',
        default_options : [
          'cpp_args=-Wall -Werror=format-security -Wno-unused-parameter',
          'cpp_std=gnu++14',
          'optimization=2',
          'warning_level=3',
          'werror=true'
        ],
        license : 'Apache-2.0',
        version : '0.0.1')

cxx = meson.get_compiler('cpp')

conf_data = configuration_data()
conf_data.set_quoted('APPNAME', 'Wahjam2')
conf_data.set_quoted('ORGNAME', 'Wahjam Project')
conf_data.set_quoted('ORGDOMAIN', 'wahjam.org')
conf_data.set_quoted('VERSION', meson.project_version())
conf_data.set('HAVE_PA_JACK_H', cxx.has_header('pa_jack.h'))

configure_file(output : 'config.h', configuration : conf_data)

inc = include_directories('.')

qt5 = import('qt5')

dependencies = [
  dependency('ogg'),
  dependency('qt5', modules : [
    'Core', 'Gui', 'Network', 'Quick', 'QuickControls2', 'Qml'
  ]),
  dependency('samplerate'),
  dependency('threads'),
  dependency('vorbis'),
  dependency('vorbisenc'),
  dependency('vorbisfile'),

  # Qt5Keychain does not having working qmake detection or pkg-config support.
  # Find it manually.
  meson.get_compiler('cpp').find_library(
      'qt5keychain',
      has_headers : ['qt5keychain/keychain.h'],
  ),
]

resources = qt5.preprocess(qresources : 'resources.qrc')

subdir('audio')
subdir('core')
subdir('standalone')
subdir('tests')
subdir('vst')
