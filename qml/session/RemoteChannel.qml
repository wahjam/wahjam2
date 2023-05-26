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
        Slider {
            anchors.horizontalCenter: parent.horizontalCenter
            orientation: Qt.Vertical
            from: 0
            to: 1.5
            value: channel.gain
            width: 20
            height: 120

            onMoved: channel.gain = value

            background: VerticalBar {
                anchors.horizontalCenter: parent.horizontalCenter
                enabled: channel.monitorEnabled
                from: 0
                to: 1
                value: channel.peakVolume
            }
        }
        MuteButton {
            anchors.horizontalCenter: parent.horizontalCenter
            checked: !channel.monitorEnabled
            onClicked: {
                channel.monitorEnabled = !channel.monitorEnabled
            }
        }
/* TODO implement solo
        SoloButton {
            anchors.horizontalCenter: parent.horizontalCenter
        }
*/
/* Enable this when more controls are needed
        MoreButton {
            anchors.horizontalCenter: parent.horizontalCenter
            target: sidePanel
        }
*/
    }
/*
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
*/
}
