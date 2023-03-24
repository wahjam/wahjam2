import QtQuick
import QtQuick.Controls
import org.wahjam.client

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
