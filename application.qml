import QtQuick.Layouts 1.3
import com.aucalic.client 1.0

StackLayout {
    id: stack

    LoginScreen {
        jamApiManager: jamApiManager

        onLoggedIn: {
            mainScreen.refreshSessionList()
            stack.currentIndex = 1
        }
    }

    MainScreen {
        id: mainScreen
        jamApiManager: jamApiManager
    }

    // Our REST API authentication
    JamApiManager {
        id: jamApiManager
    }
}
