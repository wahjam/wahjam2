// SPDX-License-Identifier: Apache-2.0
/*
 * VerticalBar - a vertical progress bar (for VU meters)
 *
 * Similar to QtQuick.Controls ProgressBar, except vertical.
 */
import QtQuick 2.14

Rectangle {
    property real value: 0
    property real from: 0
    property real to: 1

    width: 20
    height: 120
    border.color: 'black'

    Rectangle {
        x: 2
        y: 2 + (parent.height - 4) * (1 - (value - from) / (to - from))
        width: parent.width - 4
        height: (parent.height - 4) * (value - from) / (to - from)
        color: 'black'
    }
}
