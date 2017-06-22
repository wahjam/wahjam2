import QtQuick 2.3
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import com.aucalic.client 1.0

ColumnLayout {
    id: column
    property JamApiManager jamApiManager

    function refreshSessionList() {
        sessionListModel.refresh()
    }

    TabBar {
        id: bar
        width: parent.width
        currentIndex: 1
        TabButton {
            enabled: false
            text: qsTr("Session")
        }
        TabButton {
            text: qsTr("Browser")
        }
        TabButton {
            text: qsTr("Setup")
        }
    }

    StackLayout {
        width: parent.width
        currentIndex: bar.currentIndex

        Item {
            id: sessionTab

            Text { text: qsTr("Session") }
        }
        Item {
            id: browserTab

            ListView {
                model: SessionListModel {
                    id: sessionListModel
                    jamApiManager: column.jamApiManager
                }
                delegate: Column {
                    Text { text: server }
                    Text { text: topic }
                    Text { text: tempo }
                    Text { text: slots }
                    Text { text: users }
                }
            }
        }
        Item {
            id: setupTab

            Text { text: qsTr("Setup...") }
        }
    }
}
