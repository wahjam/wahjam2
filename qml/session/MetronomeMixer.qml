// SPDX-License-Identifier: Apache-2.0
/*
 * MetronomeMixer - Mixer control for the metronome
 */
import QtQuick 2.14
import QtQuick.Controls 2.14

Item {
    width: 100
    height: 200

    Text {
        id: label
        width: parent.width
        horizontalAlignment: Text.AlignHCenter
        text: qsTr("Metronome")
    }
    VerticalBar {
        id: vuMeter
        anchors.top: label.bottom
        from: -100
        to: 10
        value: -6
    }
    Button {
        anchors.top: vuMeter.bottom
        text: qsTr("Mute")
    }
}
