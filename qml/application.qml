// SPDX-License-Identifier: Apache-2.0
import QtQuick.Layouts

StackLayout {
    anchors.fill: parent
    id: stack

    LoginScreen {
        onLoggedIn: {
            stack.currentIndex = 1
            mainScreen.entered()
        }
    }

    MainScreen {
        id: mainScreen
    }
}
