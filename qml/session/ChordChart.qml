// SPDX-License-Identifier: Apache-2.0
/*
 * ChordChart - chord chart and beat display
 */

import QtQuick 2.14
import QtQuick.Controls 2.14
import org.wahjam.client 1.0

Item {
    Label {
        anchors.centerIn: parent
        text: Client.session.metronome.beat + '/' +
              Client.session.metronome.bpi + ' ' +
              Client.session.metronome.bpm + ' BPM'
    }
}
