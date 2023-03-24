// SPDX-License-Identifier: Apache-2.0
/*
 * MixerGroup - A container for related mixer channels
 */
import QtQuick
import QtQuick.Controls

Column {
    default property alias content: channels.children
    property string name: qsTr("Group")

    visible: content.length > 0
    width: channels.width

    Text {
        id: label
        width: parent.width
        horizontalAlignment: Text.AlignHCenter
        text: name
    }

    Row {
        id: channels
        spacing: 8
    }
}
