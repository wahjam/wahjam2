# There can optionally be a sub-directory called <appname>/ containing a
# Windows resource file and application icon:
# - appicon.rc
# - appicon.ico
appicon_file = appname / 'appicon.ico'
if fs.is_file(appicon_file)
    windows = import('windows')
    rc_file = appname / 'appicon.rc'
    resources += windows.compile_resources(rc_file, depend_files : [appicon_file])
endif
