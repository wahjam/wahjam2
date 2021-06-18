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

        AutoSizedListView {
            model: Client.session.localChannels
            orientation: ListView.Horizontal
            spacing: 8
            delegate: LocalChannel {
                channel: modelData
            }
        }
    }

    AutoSizedListView {
        model: Client.session.remoteUsers
        orientation: ListView.Horizontal
        spacing: 8
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
