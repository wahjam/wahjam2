// SPDX-License-Identifier: Apache-2.0
/*
 * ChordChart - chord chart and beat display
 */

import QtQuick
import QtQuick.Controls
import org.wahjam.client

Item {
    Label {
        anchors.centerIn: parent
        text: Client.session.metronome.beat + '/' +
              Client.session.metronome.bpi + ' ' +
              Client.session.metronome.bpm + ' BPM'
    }
}
