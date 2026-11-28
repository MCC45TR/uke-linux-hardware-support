// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

SettingsSection {
    id: panel
    required property var settings
    readonly property var torch: settings.flashlight
    readonly property bool available: torch.available && !torch.pending && !settings.busy && !backend.readOnly && !backend.simulation
    function toggleFlashlight() {
        if (available)
            settings.setFlashlight(!torch.on)
    }

    FormCard.FormSwitchDelegate {
        objectName: "flashlightSwitch"
        text: i18n("Flashlight")
        checked: panel.torch.on
        enabled: panel.available
        onToggled: panel.settings.setFlashlight(checked)
        Accessible.onPressAction: panel.toggleFlashlight()
        Accessible.onToggleAction: panel.toggleFlashlight()
    }

    ColumnLayout {
        Layout.fillWidth: true
        Layout.margins: Kirigami.Units.largeSpacing
        spacing: Kirigami.Units.smallSpacing

        RowLayout {
            Layout.fillWidth: true
            Controls.Label {
                Layout.fillWidth: true
                text: i18n("Brightness level")
                wrapMode: Text.Wrap
            }
            Controls.Label {
                text: panel.torch.level > 0 ? i18nc("percentage", "%1%", Math.round(level.value)) : i18n("Not reported")
            }
        }
        Controls.Slider {
            id: level
            objectName: "flashlightLevel"
            Layout.fillWidth: true
            Layout.minimumHeight: Kirigami.Units.gridUnit * 2
            enabled: panel.available && panel.torch.step > 0
            from: panel.torch.step || 50
            to: 100
            stepSize: from
            snapMode: Controls.Slider.SnapAlways
            value: panel.torch.level || from
            Accessible.name: i18n("Brightness level")
            onPressedChanged: if (!pressed && enabled && value !== panel.torch.level)
                panel.settings.setFlashlightBrightness(Math.round(value))
            onMoved: if (!pressed && enabled)
                panel.settings.setFlashlightBrightness(Math.round(value))
        }
    }

    SettingsNote {
        text: panel.torch.error || (panel.settings.busy ? i18n("Reading…") :
              panel.torch.available && panel.torch.step ? i18n("Switching brightness turns the flashlight on. This device provides two brightness levels.") :
              i18n("Flashlight status is currently unavailable."))
    }
}
