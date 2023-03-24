// SPDX-License-Identifier: Apache-2.0
/*
 * VerticalBar - a vertical progress bar (for VU meters)
 *
 * Similar to QtQuick.Controls ProgressBar, except vertical.
 */
import QtQuick
import QtQuick.Controls.Material

Rectangle {
    property real value: 0
    property real from: 0
    property real to: 1

    Behavior on value {
        NumberAnimation {
            duration: 25
        }
    }

    width: 20
    height: 120
    color: Material.color(Material.Grey)

    Rectangle {
        x: 0
        y: parent.height * (1 - (parent.value - parent.from) / (parent.to - parent.from))
        width: parent.width
        height: parent.height * (parent.value - parent.from) / (parent.to - parent.from)
        color: enabled ? Material.accent : Qt.darker(Material.color(Material.Grey))
    }
}
