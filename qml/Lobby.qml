// SPDX-License-Identifier: Apache-2.0
/*
 * Lobby - jam session listing
 */

import QtQuick 2.12
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls.Material 2.12
import org.wahjam.client 1.0
import 'globals.js' as Globals

Pane {
    id: lobby
    signal connectToJam(string server)

    Material.accent: Material.Red

    Component.onCompleted: {
        Client.apiManager.createPrivateJamFinished.connect((server) => {
            creatingPrivateJamPopup.close()
            connectToJam(server)
        })
        Client.apiManager.createPrivateJamFailed.connect((errmsg) => {
            creatingPrivateJamPopup.close()
            alertText.text = errmsg
            alertRectangle.visible = true
        })
    }

    // Fetch new jam sessions
    function refresh() {
        sessionListModel.refresh()
    }

    ColumnLayout {
        anchors.fill: parent

        Label {
            text: qsTr('<h2>Jam sessions</h2>')
        }

        ListView {
            id: jamSessionList
            Layout.fillWidth: true
            Layout.fillHeight: true

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

                            ToolTip.visible: isPublicMouseArea.containsMouse
                            ToolTip.text: isPublic ? qsTr('Public')
                                                   : qsTr('Private')
                            ToolTip.delay: Globals.toolTipDelay
                            ToolTip.timeout: Globals.toolTipTimeout

                            MouseArea {
                                id: isPublicMouseArea
                                anchors.fill: parent
                                hoverEnabled: true

                                // Let parent MouseArea handle clicks
                                propagateComposedEvents: true
                            }
                        }
                        Text { text: topic }
                    }
                    RowLayout {
                        width: parent.width
                        height: childrenRect.height

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

        RowLayout {
            Layout.alignment: Qt.AlignVCenter | Qt.AlignRight

            Button {
                text: qsTr('Create private jam')

                onClicked: {
                    creatingPrivateJamPopup.open()
                    Client.apiManager.createPrivateJam()
                }
            }

            Button {
                text: qsTr('Connect')
                onClicked: jamSessionList.currentItem.connect()
                highlighted: true
            }
        }

        Rectangle {
            id: alertRectangle
            visible: false
            Layout.fillWidth: true
            Layout.preferredHeight: childrenRect.height
            color: Material.background

            Row {
                Rectangle {
                    height: parent.height
                    width: 4
                    color: Material.accent
                }

                Column {
                    padding: 5

                    Label {
                        text: qsTr('<h3>Failed to create private jam</h3>')
                    }
                    Text {
                        id: alertText
                    }
                }
            }

            MouseArea {
                anchors.fill: parent
                onClicked: parent.visible = false
            }
        }
    }

    // The busy indicator when creating a private jam
    Popup {
        id: creatingPrivateJamPopup
        parent: Overlay.overlay
        x: Math.round((parent.width - width) / 2)
        y: Math.round((parent.height - height) / 2)
        width: 100
        height: 100
        modal: true
        closePolicy: Popup.NoAutoClose

        BusyIndicator {
            anchors.fill: parent
            running: parent.visible
        }
    }
}
