// SPDX-License-Identifier: Apache-2.0
import QtQuick 2.3
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import Qt.labs.settings 1.0
import org.wahjam.client 1.0

Pane {
    id: pane
    signal loggedIn

    ColumnLayout {
        function loginFinished() {
            mouseArea.cursorShape = Qt.ArrowCursor
            if (Client.apiManager.isLoggedIn()) {
                pane.loggedIn()
            }
        }

        anchors.centerIn: parent
        Component.onCompleted: Client.apiManager.loginFinished.connect(loginFinished)

        Text {
            text: qsTr("Login")
        }
        Text {
            id: errorField
            text: Client.apiManager.loginError
        }
        TextField {
            id: username
            placeholderText: qsTr("Username")

            Settings {
                category: "login"
                property alias username: username.text
            }
        }
        TextField {
            id: password
            placeholderText: qsTr("Password")
            echoMode: TextInput.Password
            onAccepted: logInButton.clicked()
        }
        Button {
            id: logInButton
            text: qsTr("Log in")
            onClicked: {
                mouseArea.cursorShape = Qt.WaitCursor
                Client.apiManager.username = username.text
                Client.apiManager.password = password.text
                Client.apiManager.login()
            }
        }
        Text {
            text: '<a href="https://jammr.net/accounts/register/">' + qsTr("Click here") + '</a> ' + qsTr("to create a new account")
            onLinkActivated: Qt.openUrlExternally(link)
            MouseArea {
                anchors.fill: parent
                cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.Arrowcursor;
                acceptedButtons: Qt.NoButton
            }
        }
        MouseArea {
            id: mouseArea
            x: 0
            y: 0
            width: parent.width
            height: parent.height
            acceptedButtons: Qt.NoButton // don't consume events
        }
    }
}
