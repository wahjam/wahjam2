// SPDX-License-Identifier: Apache-2.0
/*
 * LocalChannel - Local audio channel control for use within a MixerGroup
 */
import QtQuick 2.14
import QtQuick.Controls 2.14

Row {
    id: root
    property string name: qsTr("Local channel")

    Column {
        id: channelStrip
        Text {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: name
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
            text: qsTr("Solo")
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
            text: qsTr("Left/Mono input:")
        }
        Text {
            text: qsTr("Right input:")
        }
        Text {
            text: qsTr("Volume:")
        }
        ProgressBar {
            from: -24
            to: 24
            value: 0
        }
        Button {
            text: qsTr("Delete channel")
        }
    }
}
