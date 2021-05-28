// SPDX-License-Identifier: Apache-2.0
/*
 * MetronomeMixer - Mixer control for the metronome
 */
import QtQuick 2.14
import QtQuick.Controls 2.14
import '../globals.js' as Globals

Row {
    Column {
        Text {
            horizontalAlignment: Text.AlignHCenter
            text: qsTr('Metronome')
        }
        VerticalBar {
            anchors.horizontalCenter: parent.horizontalCenter
            from: -100
            to: 10
            value: -6
        }
        MuteButton {
            anchors.horizontalCenter: parent.horizontalCenter
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
