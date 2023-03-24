// SPDX-License-Identifier: Apache-2.0
/*
 * PortAudio settings UI
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.settings 1.0
import org.wahjam.client

GridLayout {
    columns: 2

    Label {
        text: "<h2>Audio</h2>"
        Layout.columnSpan: 2
    }

    Label {
        text: 'Status:'
    }
    Label {
        text: PortAudioEngine.running ? qsTr('Started') : qsTr('Stopped')
        color: PortAudioEngine.running ? 'green' : 'red'
        font.bold: true
    }

    Label {
        text: "Input device:"
    }
    ComboBox {
        id: inputDevice
        model: PortAudioEngine.availableInputDevices
        Layout.minimumWidth: 400

        onActivated: {
            PortAudioEngine.stop()
            PortAudioEngine.inputDevice = inputDevice.currentText
            PortAudioEngine.start()
        }
    }

    Label {
        text: "Output device:"
    }
    ComboBox {
        id: outputDevice
        model: PortAudioEngine.availableOutputDevices
        Layout.minimumWidth: 400

        onActivated: {
            PortAudioEngine.stop()
            PortAudioEngine.outputDevice = outputDevice.currentText
            PortAudioEngine.start()
        }
    }

    Label {
        text: "<h3>Advanced</h3>"
        Layout.columnSpan: 2
    }

    Label {
        text: "Sample rate (Hz):"
    }
    ComboBox {
        id: sampleRate
        model: [32000, 44100, 48000, 88200, 96000]

        onActivated: {
            PortAudioEngine.stop()
            PortAudioEngine.sampleRate = sampleRate.currentText
            PortAudioEngine.start()
        }
    }

    Label {
        text: "Buffer size:"
    }
    ComboBox {
        id: bufferSize
        model: [64, 128, 256, 512, 1024]

        onActivated: {
            PortAudioEngine.stop()
            PortAudioEngine.bufferSize = bufferSize.currentText
            PortAudioEngine.start()
        }
    }

    Label {
        text: "Audio system:"
    }
    ComboBox {
        id: audioSystem
        model: PortAudioEngine.availableHostApis
        Layout.minimumWidth: 400

        onActivated: {
            PortAudioEngine.stop()
            PortAudioEngine.hostApi = audioSystem.currentText
            PortAudioEngine.inputDevice = inputDevice.currentText
            PortAudioEngine.outputDevice = outputDevice.currentText
            PortAudioEngine.start()
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
        PortAudioEngine.stoppedUnexpectedly.connect(() => {
            console.log('PortAudioEngine stopped unexpectedly')
            settings.setValue('running', false)
            // TODO switch to settings tab and show audio section
        })

        PortAudioEngine.runningChanged.connect(() => {
            // TODO disconnect signal when application closes so final state is saved
            console.log('running changed ' + PortAudioEngine.running)
            if (PortAudioEngine.running) {
                settings.setValue('running', true)
                settings.setValue('audioSystem', PortAudioEngine.hostApi)
                settings.setValue('inputDevice', PortAudioEngine.inputDevice)
                settings.setValue('outputDevice', PortAudioEngine.outputDevice)
                settings.setValue('sampleRate', PortAudioEngine.sampleRate)
                settings.setValue('bufferSize', PortAudioEngine.bufferSize)
            }
        })

        let idx = audioSystem.find(settings.value('audioSystem', ''))
        if (idx !== -1) {
            audioSystem.currentIndex = idx
        }
        PortAudioEngine.hostApi = audioSystem.currentText

        idx = inputDevice.find(settings.value('inputDevice', ''))
        if (idx !== -1) {
            inputDevice.currentIndex = idx
        }
        PortAudioEngine.inputDevice = inputDevice.currentText

        idx = outputDevice.find(settings.value('outputDevice', ''))
        if (idx !== -1) {
            outputDevice.currentIndex = idx
        }
        PortAudioEngine.outputDevice = outputDevice.currentText

        idx = sampleRate.find(settings.value('sampleRate', '44100'));
        if (idx !== -1) {
            sampleRate.currentIndex = idx
        }
        PortAudioEngine.sampleRate = sampleRate.currentText;

        idx = bufferSize.find(settings.value('bufferSize', '256'));
        if (idx !== -1) {
            bufferSize.currentIndex = idx
        }
        PortAudioEngine.bufferSize = bufferSize.currentText;

        if (settings.value('running', false)) {
            PortAudioEngine.start()
        }
    }
}
