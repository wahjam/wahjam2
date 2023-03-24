// SPDX-License-Identifier: Apache-2.0
/*
 * JamControls - utility panel with jam controls
 */

import QtQuick
import QtQuick.Controls
import org.wahjam.client
import '../globals.js' as Globals

Row {
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
