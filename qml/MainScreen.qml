// SPDX-License-Identifier: Apache-2.0
import QtQuick 2.3
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2

Item {
    id: column

    function entered() {
        lobby.refresh()
    }

    TabBar {
        id: bar
        y: 0
        width: parent.width
        currentIndex: 1
        TabButton {
            id: sessionTabButton
            text: qsTr("Session")
        }
        TabButton {
            text: qsTr("Lobby")
        }
        TabButton {
            text: qsTr("Setup")
        }
    }

    StackLayout {
        anchors.top: bar.bottom
        anchors.bottom: parent.bottom
        width: parent.width
        currentIndex: bar.currentIndex

        Session {
            id: sessionTab
        }
        Lobby {
            id: lobby
            onConnectToJam: {
                sessionTab.connect(server)
                bar.currentIndex = 0
            }
        }
        Item {
            id: setupTab

            Text { text: qsTr("Setup...") }
        }
    }
}
