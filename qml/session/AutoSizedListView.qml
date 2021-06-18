// SPDX-License-Identifier: Apache-2.0
/*
 * AutoSizedListView - ListView that sizes itself large enough for all model items
 */
import QtQuick 2.14

ListView {
    width: {
        let newWidth = 0
        for (let i = 0; i < count; i++) {
            const item = itemAtIndex(i)
            if (orientation == ListView.Horizontal) {
                newWidth += item.width + (i > 0 ? spacing : 0)
            } else {
                newWidth = Math.max(newWidth, item.width)
            }
        }
        return newWidth
    }
    height: {
        let newHeight = 0
        for (let i = 0; i < count; i++) {
            const item = itemAtIndex(i)
            if (orientation == ListView.Vertical) {
                newHeight += item.height + (i > 0 ? spacing : 0)
            } else {
                newHeight = Math.max(newHeight, item.height)
            }
        }
        return newHeight
    }
}
