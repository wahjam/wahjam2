// SPDX-License-Identifier: Apache-2.0
/*
 * RemoteChannel - Remote audio channel control for use within a MixerGroup
 */
import QtQuick 2.14
import QtQuick.Controls 2.14

Row {
    // The RemoteChannel C++ object
    property var channel

    function cleanup() {
        channel = {name: '', monitorEnabled: false, peakVolume: 0}
        destroy()
    }

    Column {
        Text {
            horizontalAlignment: Text.AlignHCenter
            text: channel.name
        }
        VerticalBar {
            anchors.horizontalCenter: parent.horizontalCenter
            from: 0
            to: 1
            value: channel.peakVolume
        }
        MuteButton {
            anchors.horizontalCenter: parent.horizontalCenter
            checked: !channel.monitorEnabled
            onClicked: {
                channel.monitorEnabled = !channel.monitorEnabled
            }
        }
        SoloButton {
            anchors.horizontalCenter: parent.horizontalCenter
        }
        MoreButton {
            anchors.horizontalCenter: parent.horizontalCenter
            target: sidePanel
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
