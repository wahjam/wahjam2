// SPDX-License-Identifier: Apache-2.0
import QtQuick.Layouts 1.3

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
