// SPDX-License-Identifier: Apache-2.0
import QtQuick 2.3
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import org.wahjam.client 1.0

Item {
    id: column
    property JamApiManager jamApiManager

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
            jamApiManager: column.jamApiManager
        }
        Lobby {
            id: lobby
            jamApiManager: column.jamApiManager
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
