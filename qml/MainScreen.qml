// SPDX-License-Identifier: Apache-2.0
import QtQuick 2.3
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.14

Item {
    id: column

    function entered() {
        lobby.refresh()
    }

    TabBar {
        id: bar
        Material.accent: Material.Red
        Material.background: Material.Yellow
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
            icon.source: 'qrc:/icons/settings_black_24dp.svg'
            width: implicitWidth
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
