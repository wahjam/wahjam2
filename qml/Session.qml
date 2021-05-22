// SPDX-License-Identifier: Apache-2.0
/*
 * Session - jam session screen
 */

import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14
import org.wahjam.client 1.0
import 'session/'

Pane {
    id: session

    Component.onCompleted: {
        Client.session.stateChanged.connect((newState) =>
            console.log('jamSession state changed: ' + newState)
        )
        Client.session.error.connect((msg) =>
            console.log('jamSession unexpected error: ' + msg)
        )
        Client.session.topicChanged.connect((who, newTopic) =>
            editJam.topic = newTopic
        )
    }

    // Connect to a server
    function connect(server) {
        Client.session.connectToServer(server,
                                       Client.apiManager.username,
                                       Client.apiManager.hexToken)

        // TODO automatically open editJamPopup if private jam ACL is empty
    }

    RowLayout {
        anchors.fill: parent

        ColumnLayout {
            ChordChart {
                Layout.fillWidth: true
                Layout.fillHeight: true
            }

            ChannelStrip {
                Layout.fillWidth: true
                Layout.minimumHeight: session.height / 3
            }
        }

        ColumnLayout {
            JamControls {
                Layout.minimumWidth: session.width / 4
            }

            ChatBox {
                Layout.minimumWidth: session.width / 4
                Layout.fillHeight: true
            }
        }
    }

    Popup {
        id: editJamPopup
        parent: Overlay.overlay
        x: Math.round((parent.width - width) / 2)
        y: Math.round((parent.height - height) / 2)
        width: childrenRect.width
        height: childrenRect.height

        EditJam {
            id: editJam
            onTopicEditingFinished: {
                const newTopic = editJam.topic
                if (Client.session.topic != newTopic) {
                    Client.session.sendChatMessage('/topic ' + newTopic)
                }
            }
        }
    }
}
