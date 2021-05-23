// SPDX-License-Identifier: Apache-2.0
/*
 * MixerGroup - A container for related mixer channels
 */
import QtQuick 2.14
import QtQuick.Controls 2.14

Column {
    default property alias content: channels.children
    property string name: qsTr("Group")

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
