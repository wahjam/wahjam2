import QtQuick 2.14
import QtQuick.Controls 2.14

Column {
    Text {
        horizontalAlignment: Text.AlignHCenter
        text: qsTr('Output')
    }
    VerticalBar {
        anchors.horizontalCenter: parent.horizontalCenter
        from: -100
        to: 10
        value: -6
    }
}
