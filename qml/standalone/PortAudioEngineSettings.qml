// SPDX-License-Identifier: Apache-2.0
/*
 * PortAudio settings UI
 */
import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14
import Qt.labs.settings 1.0
import org.wahjam.client 1.0

GridLayout {
    columns: 2

    Label {
        text: "Audio"
        font.pointSize: 20
        Layout.columnSpan: 2
    }

    Label {
        text: "Input device:"
    }
    ComboBox {
        id: inputDevice
        model: PortAudioEngine.availableInputDevices

        onActivated: {
            PortAudioEngine.inputDevice = inputDevice.currentText
            settings.setValue('inputDevice', PortAudioEngine.inputDevice)
        }
    }

    Label {
        text: "Output device:"
    }
    ComboBox {
        id: outputDevice
        model: PortAudioEngine.availableOutputDevices

        onActivated: {
            PortAudioEngine.outputDevice = outputDevice.currentText
            settings.setValue('outputDevice', PortAudioEngine.outputDevice)
        }
    }

    CheckBox {
        text: "Play back my audio"
        Layout.columnSpan: 2
    }

    Label {
        text: "Advanced"
        font.pointSize: 16
        Layout.columnSpan: 2
    }

    Label {
        text: "Sample rate (Hz):"
    }
    ComboBox {
        id: sampleRate
        model: ["44100", "48000"]
    }

    Label {
        text: "Latency (ms):"
    }
    ComboBox {
        id: latency
        model: ["5.8", "10"]
    }

    Label {
        text: "Audio system:"
    }
    ComboBox {
        id: audioSystem
        model: PortAudioEngine.availableHostApis

        onActivated: {
            PortAudioEngine.hostApi = audioSystem.currentText
            settings.setValue('audioSystem', PortAudioEngine.hostApi)
        }
    }

    // The settings stuff is a little convoluted because ComboBox has a read-only
    // currentText field. We cannot load the string from settings and assign
    // ComboBox.currentText, so the normal Settings property alias mechanism cannot
    // be used.
    Settings {
        id: settings
        category: 'portaudio'
    }

    Component.onCompleted: {
        let idx = audioSystem.find(settings.value('audioSystem', ''))
        if (idx !== -1) {
            audioSystem.currentIndex = idx
            PortAudioEngine.hostApi = audioSystem.currentText
        }

        idx = inputDevice.find(settings.value('inputDevice', ''))
        if (idx !== -1) {
            inputDevice.currentIndex = idx
            PortAudioEngine.inputDevice = inputDevice.currentText
        }

        idx = outputDevice.find(settings.value('outputDevice', ''))
        if (idx !== -1) {
            outputDevice.currentIndex = idx
            PortAudioEngine.outputDevice = outputDevice.currentText
        }
    }
}
