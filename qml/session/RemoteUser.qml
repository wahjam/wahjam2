// SPDX-License-Identifier: Apache-2.0
/*
 * RemoteUser - Remote user mixer group
 */
import QtQuick 2.14

Column {
    // The RemoteUser C++ object
    property var user

    Text {
        text: user.username
    }

    AutoSizedListView {
        model: user.channels
        orientation: ListView.Horizontal
        spacing: 8
        delegate: RemoteChannel {
            channel: modelData
        }
    }

    function cleanup() {
        user = {username: '', channels: []}
        destroy()
    }
}
