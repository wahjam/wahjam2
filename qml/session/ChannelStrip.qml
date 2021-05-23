// SPDX-License-Identifier: Apache-2.0
/*
 * ChannelStrip - mixer user interface
 */

import QtQuick 2.14
import QtQuick.Controls 2.14

Row {
    spacing: 8

    MixerGroup {
        name: ''
        MetronomeMixer {}
        OutputMixer {}
    }
}
