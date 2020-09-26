// SPDX-License-Identifier: Apache-2.0
/*
 * Session - jam session screen
 */

import QtQuick 2.12
import org.wahjam.client 1.0

Item {
    property JamApiManager jamApiManager

    JamSession {
        id: jamSession
        onStateChanged: console.log('jamSession state changed: ' + newState)
        onError: console.log('jamSession unexpected error: ' + msg)
        onChatMessageReceived: console.log('jamSession received chat message: "' + msg + '" from ' + senderUsername)
        onChatPrivMsgReceived: console.log('jamSession received private message: "' + msg + '" from ' + senderUsername)
        onChatServerMsgReceived: console.log('jamSession received server message: "' + msg + '"')
        onTopicChanged: console.log('jamSession topic changed by ' + (who || "server") + ': ' + newTopic);
    }

    // Connect to a server
    function connect(server) {
        jamSession.connectToServer(server,
                                   jamApiManager.username,
                                   jamApiManager.hexToken)
    }

    Text {
        text: qsTr("Session")
    }
}
