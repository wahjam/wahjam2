import QtQuick 2.14
import QtQuick.Controls 2.14
import org.wahjam.client 1.0

Column {
    Text {
        horizontalAlignment: Text.AlignHCenter
        text: qsTr('Output')
    }
    VerticalBar {
        anchors.horizontalCenter: parent.horizontalCenter
        from: 0
        to: 1
        value: Client.masterPeakVolume
    }
}
