// SPDX-License-Identifier: Apache-2.0
/*
 * Settings - settings screen
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.wahjam.client

ScrollView {
    padding: 12
    ScrollBar.vertical.policy: ScrollBar.AlwaysOn

    ColumnLayout {
        Loader {
            id: formatSpecificSettings
        }

        Label {
            text: qsTr('<h2>About</h2>')
        }
        Label {
            text: Qt.application.displayName + ' ' + Qt.application.version
        }
        Label {
            text: `<a href="https://${Qt.application.domain}/">Website</a>`
            onLinkActivated: (link) => Qt.openUrlExternally(link)
            MouseArea {
                anchors.fill: parent
                cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.Arrowcursor;
                acceptedButtons: Qt.NoButton
            }
        }
        Label {
            text: `<a href="${Client.logFileUrl}">Show log...</a>`
            onLinkActivated: (link) => Qt.openUrlExternally(link)
            MouseArea {
                anchors.fill: parent
                cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.Arrowcursor;
                acceptedButtons: Qt.NoButton
            }
        }
    }

    // Dynamically load the format-specific settings component. This allows
    // formats (standalone, VST plugin, etc) to display different settings UIs.
    Component.onCompleted: {
        formatSpecificSettings.source = Client.format + '/FormatSpecificSettings.qml'
    }
}
