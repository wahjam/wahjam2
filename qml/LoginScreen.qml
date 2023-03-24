// SPDX-License-Identifier: Apache-2.0
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Controls.Material
import org.wahjam.client

Pane {
    id: pane
    signal loggedIn

    Material.background: Material.Yellow

    ColumnLayout {
        function loginFinished() {
            mouseArea.cursorShape = Qt.ArrowCursor
            if (Client.apiManager.isLoggedIn()) {
                pane.loggedIn()
            }
        }

        anchors.centerIn: parent
        Component.onCompleted: Client.apiManager.loginFinished.connect(loginFinished)

        Image {
            Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
            source: 'qrc:/icons/login.png'
        }
        Text {
            Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
            Layout.maximumWidth: 400
            text: Client.apiManager.loginError
            wrapMode: Text.Wrap
        }
        TextField {
            Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
            id: username
            placeholderText: qsTr("Username")
            text: Client.apiManager.username
        }
        TextField {
            Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
            id: password
            placeholderText: qsTr("Password")
            text: Client.apiManager.password
            echoMode: TextInput.Password
            onAccepted: logInButton.clicked()
        }
        CheckBox {
            Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
            id: rememberPassword
            Material.accent: Material.Red
            text: qsTr("Remember password")
            checked: Client.apiManager.rememberPassword
        }
        Button {
            Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
            id: logInButton
            Material.accent: Material.Red
            text: qsTr("Log in")
            highlighted: true
            onClicked: {
                mouseArea.cursorShape = Qt.WaitCursor
                Client.apiManager.username = username.text
                Client.apiManager.password = password.text
                Client.apiManager.rememberPassword = rememberPassword.checked
                Client.apiManager.login()
            }
        }
        Text {
            Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
            text: '<a href="https://jammr.net/accounts/register/">' + qsTr("Click here") + '</a> ' + qsTr("to create a new account")
            onLinkActivated: Qt.openUrlExternally(link)
            MouseArea {
                anchors.fill: parent
                cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.Arrowcursor;
                acceptedButtons: Qt.NoButton
            }
        }
        MouseArea {
            Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
            id: mouseArea
            x: 0
            y: 0
            width: parent.width
            height: parent.height
            acceptedButtons: Qt.NoButton // don't consume events
        }
    }
}
