// SPDX-License-Identifier: Apache-2.0
import QtQuick 2.14

Row {
    width: 800
    height: 600

    MixerGroup {
        name: qsTr("Me")

        LocalChannel {
            name: qsTr("Guitar")
        }
        LocalChannel {
            name: qsTr("Drums")
        }
    }
    MixerGroup {
        name: qsTr("Alice")

        RemoteChannel {
            name: qsTr("Bass")
        }
    }
    MixerGroup {
        name: qsTr("Bob")

        RemoteChannel {
            name: qsTr("Keys")
        }
    }
    MixerGroup {
        name: qsTr("Charles")

        RemoteChannel {
            name: qsTr("Guitar")
        }
    }
    MixerGroup {
        name: ""
        MetronomeMixer {
        }
        OutputMixer {
        }
    }
}
