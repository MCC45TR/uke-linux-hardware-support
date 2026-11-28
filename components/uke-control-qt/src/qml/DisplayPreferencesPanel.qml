// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

TileGrid {
    id: panel
    required property var settings
    required property var appWindow
    readonly property var reading: settings.desktopDisplay
    readonly property bool canChange: !backend.readOnly && !backend.simulation && !reading.busy
    function outputDescription(name) {
        if (name.startsWith("DSI-") || name.startsWith("eDP-"))
            return i18n("Built-in display · %1", name)
        if (name.startsWith("DP-") || name.startsWith("HDMI-"))
            return i18n("External display · %1", name)
        if (name.startsWith("Writeback-"))
            return i18n("Screen capture · %1", name)
        return name
    }
    minimumTileWidth: Kirigami.Units.gridUnit * 25

    SettingsSection {
        heading: i18n("Screen brightness")
        ColumnLayout {
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.largeSpacing
            RowLayout {
                Layout.fillWidth: true
                Controls.Label { Layout.fillWidth: true; text: i18n("Built-in display brightness"); wrapMode: Text.Wrap }
                Controls.Label {
                    text: panel.reading.manualBrightness >= 0 ? i18nc("percentage", "%1%", Math.round(brightness.value)) : i18n("Not reported")
                }
            }
            Controls.Slider {
                id: brightness
                objectName: "screenBrightness"
                Layout.fillWidth: true
                Layout.minimumHeight: Kirigami.Units.gridUnit * 2
                enabled: panel.canChange && panel.reading.manualBrightness >= 0
                from: 1
                to: 100
                stepSize: 1
                value: Math.max(1, panel.reading.manualBrightness)
                Accessible.name: i18n("Built-in display brightness")
                onMoved: if (!pressed && enabled) panel.settings.setScreenBrightness(Math.round(value))
                onPressedChanged: if (!pressed && enabled && value !== panel.reading.manualBrightness)
                    panel.settings.setScreenBrightness(Math.round(value))
            }
        }
        FormCard.FormSwitchDelegate {
            text: i18n("Automatic brightness")
            description: i18n("Adjust brightness to the light around you.")
            enabled: panel.canChange && panel.reading.available && typeof panel.reading.brightness === "boolean"
            checked: panel.reading.brightness === true
            onToggled: panel.settings.setDisplayPreference("desktop.brightness", checked ? "on" : "off")
            Accessible.onPressAction: if (enabled) panel.settings.setDisplayPreference("desktop.brightness", checked ? "off" : "on")
            Accessible.onToggleAction: if (enabled) panel.settings.setDisplayPreference("desktop.brightness", checked ? "off" : "on")
        }
    }

    SettingsSection {
        heading: i18n("Orientation and refresh rate")
        FormCard.FormSwitchDelegate {
            text: i18n("Automatic rotation")
            description: i18n("Rotate the display when you turn the tablet.")
            enabled: panel.canChange && panel.reading.available && typeof panel.reading.rotation === "boolean"
            checked: panel.reading.rotation === true
            onToggled: panel.settings.setDisplayPreference("desktop.rotation", checked ? "on" : "off")
            Accessible.onPressAction: if (enabled) panel.settings.setDisplayPreference("desktop.rotation", checked ? "off" : "on")
            Accessible.onToggleAction: if (enabled) panel.settings.setDisplayPreference("desktop.rotation", checked ? "off" : "on")
        }
        FormCard.FormButtonDelegate {
            text: i18n("Resolution, refresh rate and scaling")
            description: i18n("Open the KDE display settings.")
            enabled: !backend.readOnly && panel.settings.nativeLinks(2).some(item => item.id === "kcm_kscreen")
            onClicked: panel.settings.openNative("kcm_kscreen")
        }
        FormCard.FormButtonDelegate {
            text: i18n("Minimum variable refresh rate")
            description: panel.reading.liveVrr ? i18n("Change the driver’s refresh floor while the display is running.")
                                               : i18n("The current display driver applies the saved minimum on the next boot.")
            enabled: !backend.readOnly && (panel.settings.topic.changes || []).some(item => item.id === "display.vrr-set")
            onClicked: panel.appWindow.openGuide("display.vrr-set")
        }
        SettingsNote {
            visible: panel.reading.message.length > 0
            text: panel.reading.message
        }
    }

    SettingsSection {
        heading: i18n("Display performance")
        FormCard.FormSwitchDelegate {
            objectName: "displayOverlay"
            text: i18n("FPS and refresh overlay")
            description: i18n("Show KWin frame rate and measured DPU refresh over other windows.")
            checked: panel.reading.overlayActive === true
            enabled: panel.canChange && panel.reading.overlayAvailable && !panel.reading.overlayPending
            onToggled: panel.settings.setDisplayOverlay(checked)
        }
        SettingsNote {
            visible: !panel.reading.overlayAvailable
            text: i18n("The floating display monitor is available in Plasma Wayland.")
        }
        FormCard.FormButtonDelegate {
            objectName: "readDisplayResources"
            text: i18n("Read display engine resources")
            enabled: panel.canChange && !panel.settings.busy
            onClicked: panel.settings.readDisplayResources()
        }
        SettingsNote {
            visible: panel.settings.displayResources.read && !panel.settings.displayResources.available
            text: i18n("The display driver did not expose a resource map.")
        }
        Repeater {
            model: panel.settings.displayResources.available ? panel.settings.displayResources.resources : []
            delegate: SafeText {
                required property var modelData
                text: modelData.type === "dsc" ? i18n("DSC %1", modelData.index) : i18n("Layer mixer %1", modelData.index)
                description: modelData.occupied
                    ? (modelData.outputs.length ? modelData.outputs.map(name => panel.outputDescription(name)).join(", ") : modelData.crtc)
                    : i18n("Available")
            }
        }
    }

    SettingsSection {
        heading: i18n("Ambient colour profiles")
        FormCard.FormButtonDelegate {
            objectName: "selectAmbientP3"
            visible: panel.reading.ambientP3 === true
            text: i18n("Factory P3 · D65")
            description: i18n("Use the panel’s built-in colour profile with ambient white balance. Applies immediately.")
            enabled: panel.canChange
            onClicked: panel.settings.setDisplayPreference("display.ambient-color", "factory-p3")
        }
        SettingsNote {
            visible: !panel.reading.ambientP3
            text: i18n("This display does not expose a factory profile with ambient colour support.")
        }
    }

    SettingsSection {
        heading: i18n("Experiments")
        SafeText {
            text: i18n("Share DSC slots with DisplayPort when the cover is closed")
            description: panel.reading.dscSharing
                ? i18n("Enabled in the driver. The built-in display must be inactive before DisplayPort can claim its slots.")
                : i18n("This kernel controls DSC sharing at module load. A live switch requires a driver interface.")
        }
    }
}
