# SPDX-License-Identifier: Apache-2.0
project('wahjam2', 'cpp',
        default_options : [
          'cpp_args=-Wall -Werror=format-security -Wno-unused-parameter',
          'cpp_std=gnu++17',
          'optimization=2',
          'warning_level=3',
          'werror=true'
        ],
        license : 'Apache-2.0',
        version : '0.0.1')

cxx = meson.get_compiler('cpp')

# qtkeychain installs CMake files
qt6keychain = dependency('qt6keychain', method : 'cmake', required : false)
if not qt6keychain.found()
  # Qt6Keychain does not having working qmake detection or pkg-config support.
  # Fall back to manual detection if CMake didn't work.
  qt6keychain = cxx.find_library(
    'qt6keychain',
    has_headers : ['qt6keychain/keychain.h'],
    required : false,
  )
endif

conf_data = configuration_data()
conf_data.set_quoted('APPNAME', 'Wahjam2')
conf_data.set_quoted('ORGNAME', 'Wahjam Project')
conf_data.set_quoted('ORGDOMAIN', 'wahjam.org')
conf_data.set_quoted('VERSION', meson.project_version())
conf_data.set('HAVE_PA_JACK_H', cxx.has_header('pa_jack.h'))
conf_data.set('HAVE_QT6KEYCHAIN_H', qt6keychain.found())

configure_file(output : 'config.h', configuration : conf_data)

inc = include_directories('.')

qt6 = import('qt6')

dependencies = [
  dependency('ogg'),
  dependency('qt6', modules : [
    'Core', 'Gui', 'Network', 'Quick', 'QuickControls2', 'Qml'
  ]),
  dependency('samplerate'),
  dependency('threads'),
  dependency('vorbis'),
  dependency('vorbisenc'),
  dependency('vorbisfile'),
  qt6keychain,
]

resources = qt6.preprocess(qresources : 'resources.qrc')

subdir('audio')
subdir('core')
subdir('standalone')
subdir('tests')
subdir('vst')
