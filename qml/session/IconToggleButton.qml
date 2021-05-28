// SPDX-License-Identifier: Apache-2.0
/*
 * IconToggleButton - A toggle button with on/off icons and tooltips
 */
import QtQuick 2.14
import QtQuick.Controls 2.14
import '../globals.js' as Globals

Button {
    property string onIconSource
    property string offIconSource
    property string onToolTipText
    property string offToolTipText

    icon.source: checked ? onIconSource : offIconSource
    flat: true
    checkable: true
    checked: false
    hoverEnabled: true
    ToolTip.visible: hovered
    ToolTip.text: checked ? onToolTipText : offToolTipText
    ToolTip.delay: Globals.toolTipDelay
    ToolTip.timeout: Globals.toolTipTimeout
}
