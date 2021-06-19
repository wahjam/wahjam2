// SPDX-License-Identifier: Apache-2.0
/*
 * MetronomeMixer - Mixer control for the metronome
 */
import QtQuick 2.14
import QtQuick.Controls 2.14
import org.wahjam.client 1.0
import '../globals.js' as Globals

Row {
    Column {
        Text {
            horizontalAlignment: Text.AlignHCenter
            text: qsTr('Metronome')
        }
        Slider {
            anchors.horizontalCenter: parent.horizontalCenter
            width: background.width
            height: background.height
            orientation: Qt.Vertical
            from: 0
            to: 1.5
            value: Client.session.metronome.gain

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
