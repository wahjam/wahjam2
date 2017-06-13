import QtQuick 2.3
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import com.aucalic.qml 1.0

ColumnLayout {
    width: parent.width
    height: parent.height

    TabBar {
        id: bar
        width: parent.width
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

            Text { text: qsTr("Browser") }
        }
        Item {
            id: setupTab

            Text { text: qsTr("Setup...") }
        }
    }
}
