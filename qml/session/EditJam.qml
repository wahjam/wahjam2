// SPDX-License-Identifier: Apache-2.0
/*
 * EditJam - jam session configuration page
 */

import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14

ColumnLayout {
    property alias topic: topicTextInput.text

    // Fires when the topic has been edited. Careful, it may still have the
    // same value as before, so the caller needs to check for changes.
    signal topicEditingFinished()

    anchors.left: parent.left
    anchors.right: parent.right

    Label {
        text: qsTr('<h3>Topic</h3>')
    }
    TextField {
        id: topicTextInput
        Layout.fillWidth: true
        onEditingFinished: topicEditingFinished()
    }

    // TODO access control list
}
