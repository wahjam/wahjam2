import QtQuick 2.3
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2

Pane {
    ColumnLayout {
        Text{
            text: qsTr("Login")
        }
        TextField {
            placeholderText: qsTr("Username")
        }
        TextField {
            placeholderText: qsTr("Password")
            echoMode: TextInput.Password
        }
        Button {
            text: qsTr("Log in")
        }
        Text {
            text: '<a href="https://aucalic.com/accounts/register/">' + qsTr("Click here") + '</a> ' + qsTr("to create a new account")
            onLinkActivated: Qt.openUrlExternally(link)
        }
    }
}
