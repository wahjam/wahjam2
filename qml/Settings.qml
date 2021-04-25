// SPDX-License-Identifier: Apache-2.0
/*
 * Settings - settings screen
 */
import QtQuick 2.14
import org.wahjam.client 1.0

Item {
    Loader {
        id: formatSpecificSettings
    }

    // Dynamically load the format-specific settings component. This allows
    // formats (standalone, VST plugin, etc) to display different settings UIs.
    Component.onCompleted: {
        console.log('format: ' + Client.format)
        console.log('foo: ' + Client.foo)
        formatSpecificSettings.source = Client.format + '/FormatSpecificSettings.qml'
    }
}
