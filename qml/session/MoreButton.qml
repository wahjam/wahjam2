// SPDX-License-Identifier: Apache-2.0
/*
 * MoreButton - A button toggle additional controls (typically a panel with
 * advanced settings)
 */
import QtQuick
import QtQuick.Controls
import '../globals.js' as Globals

Button {
    // The additional controls. target.visible is set to true when the button
    // is clicked and set to false when it is clicked again.
    property Item target

    // The tool tip text for the button.
    property string toolTipText: qsTr('More')

    icon.source: 'qrc:/icons/more_horiz_black_24dp.svg'
    flat: true
    onClicked: {
        let show = !target.visible;
        highlighted = show;
        target.visible = show;
    }
    hoverEnabled: true
    ToolTip.visible: hovered
    ToolTip.text: toolTipText
    ToolTip.delay: Globals.toolTipDelay
    ToolTip.timeout: Globals.toolTipTimeout
}
