// SPDX-License-Identifier: Apache-2.0
/*
 * ChatBox - text chat user interface
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import org.wahjam.client

Item {
    Component.onCompleted: {
        Client.session.chatMessageReceived.connect((senderUsername, msg) =>
            chatMessages.append({username: senderUsername, message: msg})
        )
        Client.session.chatPrivMsgReceived.connect((senderUsername, msg) =>
            // TODO mark private messages to distinguish them from public messages
            chatMessages.append({username: senderUsername, message: msg})
        )
        Client.session.chatServerMsgReceived.connect((msg) =>
            // TODO mark server messages to distinguish them from public messages
            chatMessages.append({username: "SERVER", message: msg})
        )
        Client.session.topicChanged.connect((who, newTopic) => {
            console.log('jamSession topic changed by ' + (who || "server") + ': ' + newTopic)
        })
    }

    ListModel {
        id: chatMessages
    }

    ColumnLayout {
        anchors.fill: parent

        ListView {
            Layout.fillHeight: true
            Layout.fillWidth: true
            clip: true
            model: chatMessages
            spacing: 8
            id: messageList

            delegate: Column {
                Label {
                    text: username
                    textFormat: Text.PlainText
                    font.bold: true
                }
                Label {
                    width: messageList.width
                    wrapMode: Text.Wrap
                    text: message
                    textFormat: Text.MarkdownText
                    // TODO security: filter message to block undesirable markup
                }
            }

            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AlwaysOn
            }
        }

        TextField {
            Layout.fillWidth: true
            placeholderText: qsTr('Enter chat message...')
            onAccepted: {
                Client.session.sendChatMessage(text)
                clear()
            }
        }
    }
}
