// SPDX-License-Identifier: Apache-2.0
/*
 * RemoteUser - Remote user mixer group
 */
import QtQuick 2.14

MixerGroup {
    // The RemoteUser C++ object
    property var user

    name: user.username

    Repeater {
        model: user.channels
        delegate: RemoteChannel {
            channel: modelData
        }
    }
}
