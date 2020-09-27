// SPDX-License-Identifier: Apache-2.0
/*
 * Session - jam session screen
 */

import QtQuick 2.12
import org.wahjam.client 1.0

Item {
    Component.onCompleted: {
        Client.session.stateChanged.connect((newState) => console.log('jamSession state changed: ' + newState));
        Client.session.error.connect((msg) => console.log('jamSession unexpected error: ' + msg));
        Client.session.chatMessageReceived.connect((senderUsername, msg) => console.log('jamSession received chat message: "' + msg + '" from ' + senderUsername));
        Client.session.chatPrivMsgReceived.connect((senderUsername, msg) => console.log('jamSession received private message: "' + msg + '" from ' + senderUsername));
        Client.session.chatServerMsgReceived.connect((msg) => console.log('jamSession received server message: "' + msg + '"'));
        Client.session.topicChanged.connect((who, newTopic) => console.log('jamSession topic changed by ' + (who || "server") + ': ' + newTopic));
    }

    // Connect to a server
    function connect(server) {
        Client.session.connectToServer(server,
                                       Client.apiManager.username,
                                       Client.apiManager.hexToken)
    }

    Text {
        text: qsTr("Session")
    }
}
