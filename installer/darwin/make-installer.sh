#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
#
# Create a macOS installer. Set the application name, organization name, and
# organization domain name through the appname, orgname, and orgdomain
# environment variables.
#
# Usage: make-installer.sh <build-dir>

set -e

: ${appname:=wahjam2}
: ${orgname:=Wahjam Project}
: ${orgdomain:=wahjam.org}

# Ensure the build environment and Python user bin directory are in PATH
PYTHON_MAJ_MIN=$(python3 --version | cut -d' ' -f2 | cut -d. -f1,2)
export PATH="$HOME/wahjam2-mac-build/root/bin:$HOME/Library/Python/$PYTHON_MAJ_MIN/bin:$PATH"

script_dir=$(dirname $0)
src_dir=$(realpath "$script_dir/../..")
build_dir=$(realpath "$1")
app_dir="/tmp/$appname.app"

cd "$src_dir"
rm -rf "$build_dir"
rm -rf "$app_dir"
meson setup --prefix "$app_dir" \
            --bindir=Contents/MacOS \
            -D appname="$appname" \
            -D orgname="$orgname" \
            -D orgdomain="$orgdomain" \
            "$build_dir"
meson install -C "$build_dir"
macdeployqt "$app_dir" -qmldir="$src_dir/qml"
