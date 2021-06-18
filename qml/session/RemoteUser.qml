// SPDX-License-Identifier: Apache-2.0
/*
 * RemoteUser - Remote user mixer group
 */
import QtQuick 2.14

MixerGroup {
    // The RemoteUser C++ object
    property var user

    name: user.username

    function destroyChannelControls() {
        for (let i = 0; i < content.length; i++) {
            const control = content[i]
            control.cleanup()
        }
    }

    function cleanup() {
        user.channelsChanged.disconnect(updateChannels)
        destroyChannelControls()
        user = {username: ''}
        destroy()
    }

    function updateChannels() {
        destroyChannelControls()

        const component = Qt.createComponent('RemoteChannel.qml')
        let controls = []
        for (const channel of user.channels) {
            controls.push(component.createObject(this, {channel: channel}))
        }
        content = controls
    }

    Component.onCompleted: {
        user.channelsChanged.connect(updateChannels)
        updateChannels()
    }
}
