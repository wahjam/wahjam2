// SPDX-License-Identifier: Apache-2.0
/*
 * Lobby - jam session listing
 */

import QtQuick 2.12
import org.wahjam.client 1.0

Item {
    id: lobby
    signal connectToJam(string server)

    // Fetch new jam sessions
    function refresh() {
        sessionListModel.refresh()
    }

    ListView {
        anchors.fill: parent
        model: SessionListModel {
            id: sessionListModel
            jamApiManager: Client.apiManager
        }
        delegate: Rectangle {
            width: parent.width
            height: childrenRect.height
            color: ListView.view.currentIndex == index ? 'lightgray' : 'white';
            Column {
                Text { text: topic }
                Text { text: tempo }
                Text { text: slots }
                Text { text: users }
            }
            MouseArea {
                anchors.fill: parent
                onClicked: parent.ListView.view.currentIndex = index
                onDoubleClicked: {
                    lobby.connectToJam(server)
                }
            }
        }
    }
}
