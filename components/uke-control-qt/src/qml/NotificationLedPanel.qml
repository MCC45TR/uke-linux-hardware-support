// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

TileGrid {
    id: panel
    required property var settings
    readonly property var reading: settings.notificationLed
    minimumTileWidth: Kirigami.Units.gridUnit * 25

    SettingsSection {
        heading: i18n("Automatic light rules")
        FormCard.FormSwitchDelegate {
            text: i18n("Blue light during camera use")
            checked: panel.reading.camera
            enabled: false
            description: panel.reading.camera ? i18n("Managed by the camera driver.") : i18n("The camera driver has not reported an indicator.")
        }
        FormCard.FormSwitchDelegate {
            text: i18n("Light for notifications")
            enabled: false
            description: i18n("The desktop has no notification light rule for this device.")
        }
        FormCard.FormSwitchDelegate {
            text: i18n("Light when the battery is low")
            checked: panel.reading.battery
            enabled: false
            description: i18n("The battery warning light is managed by the kernel.")
        }
    }

    SettingsSection {
        heading: i18n("Light test")
        FormCard.FormButtonDelegate {
            objectName: "openLedWheel"
            text: i18n("Choose a colour")
            description: i18n("Drag on the wheel to try the colour on the tablet.")
            enabled: panel.reading.available && !backend.readOnly && !backend.simulation
            onClicked: picker.open()
        }
        SettingsNote {
            visible: text.length > 0
            text: panel.reading.error || (panel.reading.available || panel.settings.busy ? "" : i18n("The notification light is unavailable."))
        }
    }

    readonly property FormCard.FormCardDialog picker: FormCard.FormCardDialog {
        parent: Controls.Overlay.overlay
        title: i18n("Test notification light")
        width: Math.min(parent.width - Kirigami.Units.largeSpacing * 2, Kirigami.Units.gridUnit * 28)
        standardButtons: Controls.Dialog.Close
        onOpened: preview.start()
        onClosed: { preview.stop(); panel.settings.stopLedPreview() }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.largeSpacing
            spacing: Kirigami.Units.mediumSpacing
            ColorWheel {
                id: wheel
                Layout.fillWidth: true
                onColorChanged: if (panel.picker.opened && !preview.running) preview.start()
            }
            Controls.Label { text: i18n("Brightness level") }
            Controls.Slider {
                id: strength
                Layout.fillWidth: true
                Layout.minimumHeight: Kirigami.Units.gridUnit * 2
                from: 1
                to: 100
                stepSize: 1
                value: 25
                Accessible.name: i18n("Brightness level")
                onMoved: if (!preview.running) preview.start()
            }
            SettingsNote {
                visible: panel.reading.error.length > 0
                text: panel.reading.error
            }
            Timer {
                id: preview
                interval: 85
                onTriggered: panel.settings.previewLed(wheel.color, Math.round(strength.value))
            }
        }
    }
}
