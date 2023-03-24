// SPDX-License-Identifier: Apache-2.0
/*
 * MetronomeMixer - Mixer control for the metronome
 */
import QtQuick
import QtQuick.Controls
import org.wahjam.client
import '../globals.js' as Globals

Row {
    Column {
        Text {
            horizontalAlignment: Text.AlignHCenter
            text: qsTr('Metronome')
        }
        Slider {
            anchors.horizontalCenter: parent.horizontalCenter
            orientation: Qt.Vertical
            from: 0
            to: 1.5
            value: Client.session.metronome.gain
            width: 20
            height: 120

            onMoved: Client.session.metronome.gain = value

            background: VerticalBar {
                enabled: Client.session.metronome.monitor
                from: 0
                to: 1
                value: Client.session.metronome.peakVolume
            }
        }
        MuteButton {
            anchors.horizontalCenter: parent.horizontalCenter
            checked: !Client.session.metronome.monitor
            onClicked: {
                const metronome = Client.session.metronome
                metronome.monitor = !metronome.monitor
            }
        }
        MoreButton {
            anchors.horizontalCenter: parent.horizontalCenter
            target: sidePanel
            toolTipText: qsTr('More channel controls')
        }
    }
    Column {
        id: sidePanel

        visible: false

        Text {
            text: qsTr('Volume:')
        }
        Slider {
            from: -24
            to: 24
            value: 0
        }
    }
}
