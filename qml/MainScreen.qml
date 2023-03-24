// SPDX-License-Identifier: Apache-2.0
import QtQuick 2.3
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.14
import 'globals.js' as Globals

Item {
    id: column

    function entered() {
        lobby.refresh()
    }

    TabBar {
        id: bar
        Material.accent: Material.Red
        Material.background: Material.Yellow
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
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

            hoverEnabled: true
            ToolTip.visible: hovered
            ToolTip.text: qsTr('Settings')
            ToolTip.delay: Globals.toolTipDelay
            ToolTip.timeout: Globals.toolTipTimeout
        }
    }

    StackLayout {
        anchors.top: bar.bottom
        anchors.bottom: parent.bottom
        width: parent.width
        currentIndex: bar.currentIndex

        Lobby {
            id: lobby
            onConnectToJam: (server) => {
                sessionTab.connect(server)
                bar.currentIndex = 1
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
