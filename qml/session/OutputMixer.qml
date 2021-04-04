import QtQuick 2.14
import QtQuick.Controls 2.14

Column {
    Text {
        text: qsTr("Output")
    }
    VerticalBar {
        from: -100
        to: 10
        value: -6
    }
}
