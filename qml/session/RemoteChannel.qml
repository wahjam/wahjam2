// SPDX-License-Identifier: Apache-2.0
/*
 * RemoteChannel - Remote audio channel control for use within a MixerGroup
 */
import QtQuick
import QtQuick.Controls

Row {
    // The RemoteChannel C++ object
    property var channel

    Column {
        Text {
            horizontalAlignment: Text.AlignHCenter
            text: channel.name
        }
        Image {
            visible: !channel.remoteSending
            source: 'qrc:/icons/wifi_tethering_off_black_24dp.svg'
        }
        VerticalBar {
            anchors.horizontalCenter: parent.horizontalCenter
            enabled: channel.monitorEnabled
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
