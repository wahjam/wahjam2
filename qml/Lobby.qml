// SPDX-License-Identifier: Apache-2.0
/*
 * Lobby - jam session listing
 */

import QtQuick 2.12
import QtQuick.Controls 2.14
import org.wahjam.client 1.0

Pane {
    id: lobby
    signal connectToJam(string server)

    // Fetch new jam sessions
    function refresh() {
        sessionListModel.refresh()
    }

    Label {
        id: jamSessionsLabel
        anchors.top: parent.top
        text: qsTr('Jam sessions')
        font.pointSize: 20
    }

    ListView {
        id: jamSessionList
        anchors.topMargin: 12
        anchors.top: jamSessionsLabel.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: createPrivateJamButton.top

        currentIndex: 0 // auto-select first jam

        model: SessionListModel {
            id: sessionListModel
            jamApiManager: Client.apiManager
        }

        delegate: Rectangle {
            width: parent.width
            height: childrenRect.height
            color: ListView.view.currentIndex == index ? 'lightgray' : 'white';
            Column {
                Row {
                    Image {
                        source: isPublic ? 'qrc:/icons/public_black_24dp.svg'
                                         : 'qrc:/icons/lock_black_24dp.svg'
                    }
                    Text { text: topic }
                }
                Row {
                    Text { text: tempo }
                    Text { text: slots }
                    Text { text: users }
                }
            }
            MouseArea {
                anchors.fill: parent
                onClicked: parent.ListView.view.currentIndex = index
                onDoubleClicked: connect()
            }

            function connect() {
                lobby.connectToJam(server)
            }
        }
    }

    Button {
        id: createPrivateJamButton
        anchors.right: connectButton.left
        anchors.bottom: parent.bottom
        anchors.rightMargin: 8

        text: qsTr('Create private jam')

        // TODO premium feature check
        // TODO join jam session and go straight to edit jam page
    }

    Button {
        id: connectButton
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        text: qsTr('Connect')
        onClicked: jamSessionList.currentItem.connect()
    }
}
