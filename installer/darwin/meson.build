if fs.is_dir(appname)
    install_data(appname / 'appicon.icns', install_dir : 'Contents/Resources')

    conf_data = configuration_data()
    conf_data.set('APPNAME', appname)
    conf_data.set('VERSION', meson.project_version())
    conf_data.set('BUNDLEIDENTIFIER', reversed_orgdomain + '.' + appname)

    configure_file(input : 'Info.plist.in',
                   output : 'Info.plist',
                   configuration : conf_data,
                   install_dir : 'Contents',
                   install : true)
endif
