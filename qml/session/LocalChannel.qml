// SPDX-License-Identifier: Apache-2.0
/*
 * LocalChannel - Local audio channel control for use within a MixerGroup
 */
import QtQuick 2.14
import QtQuick.Controls 2.14

Row {
    property string name: qsTr('Local channel')

    Column {
        Text {
            horizontalAlignment: Text.AlignHCenter
            text: name
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
        SoloButton {
            anchors.horizontalCenter: parent.horizontalCenter
        }
        Button {
            anchors.horizontalCenter: parent.horizontalCenter
            icon.source: 'qrc:/icons/more_horiz_black_24dp.svg'
            flat: true
            onClicked: {
                let show = !sidePanel.visible;
                highlighted = show;
                sidePanel.visible = show;
            }
            hoverEnabled: true
            ToolTip.visible: hovered
            ToolTip.text: qsTr('More channel controls')
            ToolTip.delay: Globals.toolTipDelay
            ToolTip.timeout: Globals.toolTipTimeout
        }
    }
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
}
