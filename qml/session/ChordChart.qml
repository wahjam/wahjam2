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

        // See core/JamSession.h for State enum values
        text: Client.session.state == 0 ? 'Disconnected' :
              Client.session.state == 1 ? 'Connecting' :
              Client.session.state == 2 ?
              (Client.session.metronome.beat + '/' +
               Client.session.metronome.bpi + ' ' +
               Client.session.metronome.bpm + ' BPM') :
              'Disconnecting'
    }
}
