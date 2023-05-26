// SPDX-License-Identifier: Apache-2.0
/*
 * LocalChannel - Local audio channel control for use within a MixerGroup
 */
import QtQuick
import QtQuick.Controls
import '../globals.js' as Globals

Row {
    // The LocalChannel C++ object
    property var channel

    Column {
        Text {
            horizontalAlignment: Text.AlignHCenter
            text: channel.name
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
                from: 0
                to: 1
                value: channel.peakVolume
            }
        }
        SendButton {
            anchors.horizontalCenter: parent.horizontalCenter
            checked: !channel.send
            onClicked: channel.send = !channel.send
        }
/* Enable this when more channel controls are needed
        MoreButton {
            anchors.horizontalCenter: parent.horizontalCenter
            target: sidePanel
            toolTipText: qsTr('More channel controls')
        }
*/
    }
/*
    Column {
        id: sidePanel

        visible: false

        Text {
            text: qsTr('Left/Mono input:')
        }
        Text {
            text: qsTr('Right input:')
        }
        Text {
            text: qsTr('Volume:')
        }
        Slider {
            from: -24
            to: 24
            value: 0
        }
        Button {
            anchors.horizontalCenter: parent.horizontalCenter
            icon.source: 'qrc:/icons/delete_black_24dp.svg'
            flat: true
            hoverEnabled: true
            ToolTip.visible: hovered
            ToolTip.text: qsTr('Delete channel')
            ToolTip.delay: Globals.toolTipDelay
            ToolTip.timeout: Globals.toolTipTimeout
        }
    }
*/
}
