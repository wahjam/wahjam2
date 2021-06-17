// SPDX-License-Identifier: Apache-2.0
/*
 * ChannelStrip - mixer user interface
 */

import QtQuick 2.14
import QtQuick.Controls 2.14
import org.wahjam.client 1.0

Row {
    spacing: 8

    MixerGroup {
        name: Client.apiManager.username

        Component.onCompleted: {
            const session = Client.session
            const component = Qt.createComponent('LocalChannel.qml')

            session.localChannelsChanged.connect(() => {
                let controls = []
                for (const channel of session.localChannels) {
                    controls.push(component.createObject(
                        this, {channel: channel}
                    ))
                }
                content = controls
            })
        }
    }

    MixerGroup {
        name: ''
        MetronomeMixer {}
        OutputMixer {}
    }
}
