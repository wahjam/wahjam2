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
