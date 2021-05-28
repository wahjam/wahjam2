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
        LocalChannel {}
    }

    MixerGroup {
        name: ''
        MetronomeMixer {}
        OutputMixer {}
    }
}
