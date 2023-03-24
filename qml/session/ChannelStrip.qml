// SPDX-License-Identifier: Apache-2.0
/*
 * ChannelStrip - mixer user interface
 */

import QtQuick
import QtQuick.Controls
import org.wahjam.client

Row {
    spacing: 8

    MixerGroup {
        name: Client.apiManager.username

        Repeater {
            model: Client.session.localChannels
            delegate: LocalChannel {
                channel: modelData
            }
        }
    }

    Repeater {
        model: Client.session.remoteUsers
        delegate: RemoteUser {
            user: modelData
        }
    }

    MixerGroup {
        name: ''
        MetronomeMixer {}
        OutputMixer {}
    }
}
