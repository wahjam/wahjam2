// SPDX-License-Identifier: Apache-2.0
/*
 * JamControls - utility panel with jam controls
 */

import QtQuick 2.14
import QtQuick.Controls 2.14
import org.wahjam.client 1.0
import '../globals.js' as Globals

Row {
    Button {
        icon.source: checked ?
            'qrc:/icons/wifi_tethering_off_black_24dp.svg' :
            'qrc:/icons/wifi_tethering_black_24dp.svg'
        flat: true
        checkable: true
        checked: false
        hoverEnabled: true
        ToolTip.visible: hovered
        ToolTip.text: checked ?
            qsTr('Not sending audio to other users') :
            qsTr('Sending audio to other users')
        ToolTip.delay: Globals.toolTipDelay
        ToolTip.timeout: Globals.toolTipTimeout
        // TODO toggle send
    }

    Button {
        icon.source: 'qrc:/icons/room_preferences_black_24dp.svg'
        flat: true
        hoverEnabled: true
        ToolTip.visible: hovered
        ToolTip.text: qsTr('Edit jam')
        ToolTip.delay: Globals.toolTipDelay
        ToolTip.timeout: Globals.toolTipTimeout
        onClicked: editJamPopup.open()
    }

    Button {
        icon.source: 'qrc:/icons/logout_black_24dp.svg'
        flat: true
        hoverEnabled: true
        ToolTip.visible: hovered
        ToolTip.text: qsTr('Disconnect')
        ToolTip.delay: Globals.toolTipDelay
        ToolTip.timeout: Globals.toolTipTimeout
        onClicked: Client.session.disconnectFromServer()
    }
}
