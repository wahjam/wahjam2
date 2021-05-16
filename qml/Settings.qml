// SPDX-License-Identifier: Apache-2.0
/*
 * Settings - settings screen
 */
import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14
import org.wahjam.client 1.0

Pane {
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
            text: '<a href="https://' + Qt.application.domain + '/">https://' + Qt.application.domain + '/'
            onLinkActivated: Qt.openUrlExternally(link)
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
