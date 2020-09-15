import QtQuick 2.3
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import org.wahjam.client 1.0

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
                anchors.fill: parent
                model: SessionListModel {
                    id: sessionListModel
                    jamApiManager: column.jamApiManager
                }
                delegate: Item {
                    width: container.width
                    height: childrenRect.height
                    Column {
                        Text { text: topic }
                        Text { text: tempo }
                        Text { text: slots }
                        Text { text: users }
                    }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: parent.ListView.view.currentIndex = index
                    }
                }
            }
        }
        Item {
            id: setupTab

            Text { text: qsTr("Setup...") }
        }
    }
}
