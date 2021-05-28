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
            text: qsTr('Volume:')
        }
        Slider {
            from: -24
            to: 24
            value: 0
        }
    }
}
