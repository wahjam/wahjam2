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
        currentIndex: 0
        TabButton {
            text: qsTr("Lobby")
        }
        TabButton {
            id: sessionTabButton
            text: qsTr("Session")
        }
        TabButton {
            text: qsTr("Settings")
        }
    }

    StackLayout {
        anchors.top: bar.bottom
        anchors.bottom: parent.bottom
        width: parent.width
        currentIndex: bar.currentIndex

        Lobby {
            id: lobby
            onConnectToJam: {
                sessionTab.connect(server)
                bar.currentIndex = 0
            }
        }
        Session {
            id: sessionTab
        }
        Settings {
            id: setupTab
        }
    }
}
