// SPDX-License-Identifier: Apache-2.0
/*
 * MetronomeMixer - Mixer control for the metronome
 */
import QtQuick 2.14
import QtQuick.Controls 2.14

Row {
    Column {
        id: channelStrip
        Text {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: qsTr("Metronome")
        }
        VerticalBar {
            from: -100
            to: 10
            value: -6
        }
        Button {
            text: qsTr("Mute")
        }
        Button {
            text: qsTr("...")
            onClicked: {
                let show = !sidePanel.visible;
                highlighted = show;
                sidePanel.visible = show;
            }
        }
    }
    Column {
        id: sidePanel

        visible: false

        Text {
            text: qsTr("Volume:")
        }
        Slider {
            from: -24
            to: 24
            value: 0
        }
    }
}
