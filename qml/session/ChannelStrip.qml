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
                // Destroy existing LocalChannel controls
                for (let i = 0; i < content.length; i++) {
                    const control = content[i]

                    // Assign a dummy object to prevent property binding errors
                    // before the control is finally deleted.
                    control.channel = {name: '', peakVolume: 0}

                    control.destroy()
                }

                // Instantiate new LocalChannel controls
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
