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
        Client.session.chatMessageReceived.connect((senderUsername, msg) =>
            chatMessages.append({username: senderUsername, message: msg})
        );
        Client.session.chatPrivMsgReceived.connect((senderUsername, msg) =>
            // TODO mark private messages to distinguish them from public messages
            chatMessages.append({username: senderUsername, message: msg})
        );
        Client.session.chatServerMsgReceived.connect((msg) =>
            // TODO mark server messages to distinguish them from public messages
            chatMessages.append({username: "SERVER", message: msg})
        );
        Client.session.topicChanged.connect((who, newTopic) => console.log('jamSession topic changed by ' + (who || "server") + ': ' + newTopic));
    }

    // Connect to a server
    function connect(server) {
        Client.session.connectToServer(server,
                                       Client.apiManager.username,
                                       Client.apiManager.hexToken)
    }

    ListModel {
        id: chatMessages
    }

    ListView {
        id: chatMessagesListView
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: chatMessageInput.top
        model: chatMessages
        delegate: Text {
            text: "<b>" + username + "</b>: " + message
        }
        // TODO scroll bar
    }

    TextInput {
        id: chatMessageInput
        width: parent.width
        height: 20
        anchors.bottom: parent.bottom
        onAccepted: {
            Client.session.sendChatMessage(chatMessageInput.text)
            chatMessageInput.clear()
        }
    }
}
