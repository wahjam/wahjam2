// SPDX-License-Identifier: Apache-2.0
/*
 * Lobby - jam session listing
 */

import QtQuick 2.12
import org.wahjam.client 1.0

Item {
    id: lobby
    property JamApiManager jamApiManager

    // Fetch new jam sessions
    function refresh() {
        sessionListModel.refresh()
    }

    ListView {
        anchors.fill: parent
        model: SessionListModel {
            id: sessionListModel
            jamApiManager: lobby.jamApiManager
        }
        delegate: Item {
            width: parent.width
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
