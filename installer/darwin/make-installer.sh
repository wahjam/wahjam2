#!/bin/bash
# SPDX-License-Identifier: Apache-2.0

set -o errexit -o pipefail

usage() {
    echo "Usage: make-installer.sh [--appname wahjam2]"
    echo "                         [--orgname \"Wahjam Project\"]"
    echo "                         [--orgdomain wahjam.org]"
    echo "                         [--apple-id <apple-id>]"
    echo "                         [--team-id <team-id>]"
    echo "                         [--keychain-profile <profile>]"
    echo "                         <build-dir>"
    echo
    echo "Create a macOS installer dmg file."
    echo
    echo "REQUIRED OPTIONS"
    echo "--appname <appname>      Application name for executable, dmg file, window titles, etc"
    echo "--orgname <orgname>      Organization name for application metadata"
    echo "--orgdomain <orgdomain>  Domain name of organization and application server"
    echo
    echo "CODE SIGNING (Optional)"
    echo "--apple-id <apple-id>    Apple ID"
    echo "--team-id <team-id>      Team ID"
    echo "--keychain-profile       Profile name of previously saved app-specific password."
    echo "                         Save the password to the keychain with:"
    echo "                         xcrun notarytool store-credentials <profile> --apple-id <apple-id> --team-id <team-id>"
    exit 1
}

appname=wahjam2
orgname="Wahjam Project"
orgdomain=wahjam.org
apple_id=
team_id=
keychain_profile=
macdeployqt_opts=()

while [ -n "$1" ]; do
    case "$1" in
        --apple-id)
            shift
            apple_id="$1"
            ;;
        --appname)
            shift
            appname="$1"
            ;;
        --orgname)
            shift
            orgname="$1"
            ;;
        --orgdomain)
            shift
            orgdomain="$1"
            ;;
        --team-id)
            shift
            team_id="$1"
            macdeployqt_opts+=("-sign-for-notarization=$1")
            ;;
        --keychain-profile)
            shift
            keychain_profile="$1"
            ;;
        --*)
            usage
            ;;
        *)
            break
            ;;
    esac
    shift
done

# Either all or no code signing options must be given
if [ ! \( -z "$apple_id" -a -z "$team_id" -a -z "$keychain_profile" \) ]; then
    if [ -z "$apple_id" ]; then
        echo "Missing --apple-id for code signing"
        exit 1
    fi
    if [ -z "$team_id" ]; then
        echo "Missing --team-id for code signing"
        exit 1
    fi
    if [ -z "$keychain_profile" ]; then
        echo "Missing --keychain-profile for code signing"
        exit 1
    fi
fi

if [ -z "$1" ]; then
    usage
fi

# Ensure the build environment and Python user bin directory are in PATH
PYTHON_MAJ_MIN=$(python3 --version | cut -d' ' -f2 | cut -d. -f1,2)
export PATH="$HOME/wahjam2-mac-build/root/bin:$HOME/Library/Python/$PYTHON_MAJ_MIN/bin:$PATH"

# Unlock keychain for code signing, this prompts the user for a password
if [ -n "$apple_id" ]; then
    security unlock-keychain ~/Library/Keychains/login.keychain-db
fi

mkdir -p "$1" # realpath(1) requires that the path exists

script_dir=$(dirname $0)
src_dir=$(realpath "$script_dir/../..")
build_dir=$(realpath "$1")
app_dir="/tmp/$appname.app"
dmg_file="/tmp/$appname.dmg"

cd "$src_dir"
rm -rf "$build_dir"
rm -rf "$app_dir"
rm -f "$dmg_file"
meson setup --prefix "$app_dir" \
            --bindir=Contents/MacOS \
            -D appname="$appname" \
            -D orgname="$orgname" \
            -D orgdomain="$orgdomain" \
            "$build_dir"
meson install -C "$build_dir"
macdeployqt "$app_dir" "${macdeployqt_opts[@]}" -qmldir="$src_dir/qml" -dmg

if [ -n "$apple_id" ]; then
    if xcrun notarytool submit --apple-id "$apple_id" --team-id "$team_id" --keychain-profile "$keychain_profile" "$dmg_file" --wait | tee /tmp/notary.log; then
        ok=1
    else
        ok=0
    fi
    submission_id=$(awk '/  id:/ { print $2; exit; }' </tmp/notary.log)
    xcrun notarytool log --apple-id "$apple_id" --team-id "$team_id" --keychain-profile "$keychain_profile" "$submission_id"
    if [ "$ok" -eq 0 ]; then
        exit 1
    fi
    xcrun stapler staple "$dmg_file"
    echo "Successfully built $dmg_file"
fi
