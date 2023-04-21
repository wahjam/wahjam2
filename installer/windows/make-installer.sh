#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
#
# Create a Windows installer.
#
# If a Software Publisher File (spc) named "<appname>.spc" and a Private Key
# File (pvk) named "<appname>.pvk" are located in the same directory as this
# script, then code signing is performed. The private key file password must be
# in an environment variable named PVK_PASSWORD.
#
# Usage: make-windows-installer.sh <build-dir>

set -e

# Do not leak the password environment variable to subprocesses
export -n PVK_PASSWORD

: ${CC:=x86_64-w64-mingw32.shared-gcc}
: ${QMAKE:=x86_64-w64-mingw32.shared-qt6-qmake}
: ${MESON:=x86_64-w64-mingw32.shared-meson}

meson_projectinfo() {
    "$MESON" introspect --projectinfo "$build_dir" | python -c "import json; import sys; print(json.load(sys.stdin)['$1'])"
}

meson_buildoption() {
    "$MESON" introspect --buildoptions "$build_dir" | python -c "import json; import sys; print(list(filter(lambda x: x['name'] == '$1', json.load(sys.stdin)))[0]['value'])"
}

build_dir="$1"
install_dir="$build_dir/install"

appname=$(meson_buildoption appname)
version=$(meson_projectinfo version)
orgdomain=$(meson_buildoption orgdomain)

script_dir=$(dirname $0)
spc_file="$script_dir/$appname.spc"
pvk_file="$script_dir/$appname.pvk"

# System DLLs are installed here
BIN_DIR=$($CC --print-sysroot)/bin

# Qt DLLs and support files are installed here
QT_INSTALL_BINS=$($QMAKE -query QT_INSTALL_BINS)
QT_INSTALL_PLUGINS=$($QMAKE -query QT_INSTALL_PLUGINS)
QT_INSTALL_QML=$($QMAKE -query QT_INSTALL_QML)

# Library dependencies can be found with "objdump -x <FILE> | grep 'DLL Name'"
system_dlls=(
    libbz2.dll
    libfreetype-6.dll
    libgcc_s_seh-1.dll
    libglib-2.0-0.dll
    libharfbuzz-0.dll
    libiconv-2.dll
    icudt66.dll
    icuin66.dll
    icuuc66.dll
    libintl-8.dll
    libjpeg-9.dll
    libogg-0.dll
    libpcre-1.dll
    libpcre2-16-0.dll
    libpng16-16.dll
    libportaudio-2.dll
    libqt6keychain.dll
    libsamplerate-0.dll
    libstdc++-6.dll
    libvorbis-0.dll
    libvorbisenc-2.dll
    libvorbisfile-3.dll
    libwinpthread-1.dll
    libzstd.dll
    zlib1.dll
)

qt_dlls=(
    Qt6Core.dll
    Qt6Gui.dll
    Qt6LabsSettings.dll
    Qt6Network.dll
    Qt6OpenGL.dll
    Qt6Qml.dll
    Qt6QmlModels.dll
    Qt6QmlWorkerScript.dll
    Qt6Quick.dll
    Qt6QuickControls2.dll
    Qt6QuickControls2Impl.dll
    Qt6QuickLayouts.dll
    Qt6QuickTemplates2.dll
    Qt6Svg.dll
)

qt_plugin_dirs=(
    imageformats
    iconengines
    platforms
    styles
    tls
)

run() {
    echo "$@"
    "$@"
}

copy_dlls() {
    dll_dir="$1"
    dest_dir="$2"
    shift
    shift

    mkdir -p "$dest_dir"
    for f in "$@"; do
        run cp "$dll_dir/$f" "$dest_dir/"
    done
}

copy_qt_plugins() {
    dll_dir="$1"
    dest_dir="$2"
    shift
    shift

    mkdir -p "$dest_dir/plugins"
    for d in "$@"; do
        run cp -r "$dll_dir/$d" "$dest_dir/plugins/"
    done
}

# Sign EXE and DLL files with a code signing certificate.
sign() {
    file="$1"

    if [ -z "$PVK_PASSWORD" ]; then
        echo "No private key file password set in PVK_PASSWORD environment variable."
        return 1
    fi

    echo
    echo "Signing $file..."
    signcode -spc "$spc_file" \
        -v "$pvk_file" \
        -a sha256 \
        '-$' commercial \
        -n "$appname" \
        -i "https://$orgdomain/" \
        -t http://timestamp.digicert.com \
        "$file" <<<"$PVK_PASSWORD"
}

rm -rf "$install_dir"
mkdir -p "$install_dir"

run cp "$build_dir/standalone/$appname.exe" "$install_dir/"

copy_dlls "$BIN_DIR" "$install_dir" "${system_dlls[@]}"
copy_dlls "$QT_INSTALL_BINS" "$install_dir" "${qt_dlls[@]}"
#run cp "$QT_INSTALL_BINS"/*.dll "$install_dir/"
copy_qt_plugins "$QT_INSTALL_PLUGINS" "$install_dir" "${qt_plugin_dirs[@]}"

# Remove unused QML themes
run cp -r "$QT_INSTALL_QML" "$install_dir/"
run rm -rf "$install_dir/qml/QtQuick/Dialogs/quickimpl/qml/+Fusion"
run rm -rf "$install_dir/qml/QtQuick/Dialogs/quickimpl/qml/+Imagine"
run rm -rf "$install_dir/qml/QtQuick/Dialogs/quickimpl/qml/+Universal"
run rm -rf "$install_dir/qml/QtQuick/Controls/Fusion"
run rm -rf "$install_dir/qml/QtQuick/Controls/Imagine"
run rm -rf "$install_dir/qml/QtQuick/Controls/Universal"

if [ -f "$spc_file" -a -f "$pvk_file" ]; then
    for f in $(find "$install_dir" -name \*.dll -o -name \*.exe); do
        sign "$f"
    done
fi

# zipfile="$build_dir/$appname-$version.zip"
# rm -f "$zipfile"
# rm -f "$build_dir/$appname"
# ln -s "$install_dir" "$build_dir/$appname"
# cd "$build_dir"
# zip -9 -r "$zipfile" "$appname/"
# rm -f "$appname"

makensis -DPROGRAM_NAME="$appname" -DVERSION="$version" "$script_dir/installer.nsi" || true

if [ -f "$spc_file" -a -f "$pvk_file" ]; then
    sign "qtclient/installer/windows/$appname-installer.exe"
else
    echo "No code signing certificate found. Not signing executable."
fi
