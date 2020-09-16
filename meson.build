# SPDX-License-Identifier: Apache-2.0
project('wahjam2', 'cpp',
        default_options : [
          'cpp_std=gnu++14',
        ],
        license : 'Apache-2.0',
        version : '0.0.1')

inc = include_directories('.')

qt5 = import('qt5')

dependencies = [
  dependency('threads'),
  dependency('qt5', modules : ['Core', 'Network', 'Quick', 'QuickControls2', 'Qml']),
]

resources = qt5.preprocess(qresources : 'resources.qrc')

subdir('audio')
subdir('core')
subdir('standalone')
subdir('tests')
subdir('vst')
