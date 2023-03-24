// SPDX-License-Identifier: Apache-2.0
/*
 * RemoteUser - Remote user mixer group
 */
import QtQuick

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
