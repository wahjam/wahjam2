project('wahjam2', 'cpp',
        default_options : [
          'cpp_std=gnu++14',
        ],
        license : 'Apache-2.0',
        version : '0.0.1')

inc = include_directories('.')

subdir('tests')

qt5 = import('qt5')

processed = qt5.preprocess(
    moc_headers : [
        'JamApiManager.h',
        'JamConnection.h',
        'Metronome.h',
        'SessionListModel.h',
        'VstPlugin.h'
    ],
    qresources : 'resources.qrc',
)

shared_library(meson.project_name(),
    'AudioStream.cpp',
    'AudioProcessor.cpp',
    'global.cpp',
    'JamApiManager.cpp',
    'JamConnection.cpp',
    'Metronome.cpp',
    'rcu.cpp',
    'SessionListModel.cpp',
    'VstPlugin.cpp',
    processed,
    dependencies : [
        dependency('threads'),
        dependency('qt5', modules : ['Core', 'Network', 'Quick', 'QuickControls2', 'Qml']),
    ],
    name_prefix : '')
