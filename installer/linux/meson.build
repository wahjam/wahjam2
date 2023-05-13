# There can optionally be a sub-directory called <appname>/ containing desktop
# metadata and icons:
# - <reversed-orgdomain>.<appname>.desktop
# - <reversed-orgdomain>.<appname>.metainfo.xml
# - <reversed-orgdomain>.<appname>.svg
# - <reversed-orgdomain>.<appname>.png
if fs.is_dir(appname)
    basename = '@0@/@1@.@2@'.format(appname, reversed_orgdomain, appname)

    desktop_file = basename + '.desktop'
    metainfo_file = basename + '.metainfo.xml'
    svg_icon_file = basename + '.svg'
    png_icon_file = basename + '.png'

    install_data(desktop_file, install_dir : get_option('datadir') / 'applications')
    install_data(metainfo_file, install_dir : get_option('datadir') / 'metainfo')
    install_data(svg_icon_file, install_dir : get_option('datadir') / 'icons/hicolor/scalable/apps')
    install_data(png_icon_file, install_dir : get_option('datadir') / 'icons/hicolor/48x48/apps')
endif
