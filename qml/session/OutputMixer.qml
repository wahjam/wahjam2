import QtQuick 2.14
import QtQuick.Controls 2.14

Item {
    width: 100
    height: 200

    Text {
        id: label
        width: parent.width
        horizontalAlignment: Text.AlignHCenter
        text: qsTr("Output")
    }
    VerticalBar {
        id: vuMeter
        anchors.top: label.bottom
        from: -100
        to: 10
        value: -6
    }
    Button {
        id: sendButton
        anchors.top: vuMeter.bottom
        text: qsTr("Stop sending")
        highlighted: true
    }
    Button {
        id: adminButton
        anchors.top: sendButton.bottom
        text: qsTr("Admin")
    }
    Button {
        anchors.top: adminButton.bottom
        text: qsTr("Leave")
    }
}
