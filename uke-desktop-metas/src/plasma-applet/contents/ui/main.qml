// SPDX-License-Identifier: GPL-3.0-or-later
// Layout adapted from SENEMOS Nabu Tablet Control; see SOURCE.md.
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.components as PC
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.plasmoid
import org.kde.plasma.plasma5support as Plasma5Support
import org.senemos.uke.controls 1.0 as Uke
import "Control.js" as Control

PlasmoidItem {
    id: root
    property var panel: ({})
    property var desktop: ({})
    property var usb: ({})
    property var lights: ({})
    property var rgb: ({})
    property var requests: ({})
    property string errorText: ""
    property int serial: 0
    property int pending: 0
    property string requestedAction: ""

    function execute(action, values) {
        if (Control.isLightAction(action)) {
            if (lightClient.request(action, values || [])) ++pending
            else errorText = i18n("Could not contact the tablet controls service.")
            return
        }
        const command = Control.command(action, values)
        if (!command) return
        const source = "UKE_TRAY_REQUEST=" + (++serial) + " " + command
        requests[source] = action
        ++pending
        executable.connectSource(source)
    }
    function refresh() {
        if (pending > 0) return
        execute("plasma")
        execute("desktop")
        execute("usb")
        execute("lights")
        execute("rgb")
    }
    function change(action, values) {
        if (pending > 0) return
        errorText = ""
        execute(action, values)
    }
    function finish(source, data) {
        const action = requests[source]
        if (!action) return
        delete requests[source]
        --pending
        const query = action === "plasma" || action === "desktop" || action === "usb" || action === "lights" || action === "rgb"
        if (query) {
            let value = {}
            try {
                if (data["exit code"] !== 0) throw new Error("uke-control failed")
                value = Control.parse(action, String(data.stdout || ""))
            } catch (error) {
                errorText = i18n("Could not read tablet state. Refresh to try again.")
            }
            if (action === "plasma") panel = value
            else if (action === "desktop") desktop = value
            else if (action === "usb") usb = value
            else if (action === "lights") lights = value
            else rgb = value
        } else {
            if (data["exit code"] !== 0)
                errorText = i18n("The change failed or authorization was cancelled.")
            if (root.expanded) refresh()
        }
    }
    function role() {
        const ports = usb.typec || []
        for (let i = 0; i < ports.length; ++i)
            if (ports[i].name === "port0") return String(ports[i].data_role || i18n("Unavailable"))
        return i18n("Unavailable")
    }
    function canRequestRole() {
        const ports = usb.typec || []
        for (let i = 0; i < ports.length; ++i)
            if (ports[i].name === "port0") return String(ports[i].port_type).indexOf("dual") >= 0
        return false
    }
    function confirm(action) {
        requestedAction = action
        confirmation.open()
    }

    Plasmoid.icon: "input-tablet"
    Plasmoid.status: PlasmaCore.Types.ActiveStatus
    toolTipMainText: i18n("Uke Tablet Controls")
    toolTipSubText: i18n("Display, cover and USB")
    onExpandedChanged: { if (expanded) refresh(); else lightClient.close() }

    Uke.LightClient {
        id: lightClient
        onCompleted: function(action, output, error) {
            --root.pending
            if (error.length > 0) {
                if (action === "lights") root.lights = ({})
                else if (action === "rgb") root.rgb = ({})
                if (root.expanded) root.errorText = action === "lights" || action === "rgb"
                    ? i18n("Could not read tablet state. Refresh to try again.")
                    : i18n("The change failed or authorization was cancelled.")
            } else if (action === "lights" || action === "rgb") {
                try {
                    const value = Control.parse(action, output)
                    if (action === "lights") root.lights = value
                    else root.rgb = value
                } catch (error) {
                    if (action === "lights") root.lights = ({})
                    else root.rgb = ({})
                    root.errorText = i18n("Could not read tablet state. Refresh to try again.")
                }
            }
            if (action !== "lights" && action !== "rgb" && root.expanded) root.refresh()
        }
    }

    Timer {
        interval: 2000
        repeat: true
        running: root.expanded && (root.lights.flashlight?.on === true || root.rgb.mode === "test")
        onTriggered: {
            if (root.pending === 0) {
                if (root.lights.flashlight?.on === true) root.execute("lights")
                if (root.rgb.mode === "test") root.execute("rgb")
            }
        }
    }

    Plasma5Support.DataSource {
        id: executable
        engine: "executable"
        onNewData: function(source, data) {
            disconnectSource(source)
            root.finish(source, data)
        }
    }

    compactRepresentation: PC.ToolButton {
        icon.name: "input-tablet"
        Accessible.name: root.toolTipMainText
        onClicked: root.expanded = !root.expanded
    }

    fullRepresentation: PC.ScrollView {
        id: scrollView
        Layout.minimumWidth: Kirigami.Units.gridUnit * 22
        Layout.preferredWidth: Kirigami.Units.gridUnit * 26
        Layout.minimumHeight: Kirigami.Units.gridUnit * 20
        Layout.preferredHeight: Kirigami.Units.gridUnit * 30
        contentWidth: availableWidth
        QQC2.ScrollBar.horizontal.policy: QQC2.ScrollBar.AlwaysOff

        ColumnLayout {
            width: scrollView.availableWidth
            spacing: 0
            RowLayout {
                Layout.fillWidth: true
                Layout.margins: Kirigami.Units.largeSpacing
                Kirigami.Heading { Layout.fillWidth: true; text: i18n("Uke Tablet Controls"); level: 2 }
                PC.ToolButton {
                    icon.name: "view-refresh"
                    text: i18n("Refresh")
                    display: PC.AbstractButton.IconOnly
                    Accessible.name: text
                    enabled: root.pending === 0
                    onClicked: root.refresh()
                }
            }
            Kirigami.InlineMessage {
                Layout.fillWidth: true
                Layout.margins: Kirigami.Units.largeSpacing
                visible: root.errorText.length > 0
                text: root.errorText
                type: Kirigami.MessageType.Error
            }
            ControlRow {
                title: i18n("Automatic rotation")
                description: root.panel.rotation === true ? i18n("Always") : i18n("Off or tablet mode only")
                iconName: "object-rotate-right"
                available: typeof root.panel.rotation === "boolean"
                busy: root.pending > 0
                action: Component {
                    PC.Switch {
                        checked: root.panel.rotation === true
                        Accessible.name: i18n("Automatic rotation")
                        onToggled: root.change(checked ? "rotationOn" : "rotationOff")
                    }
                }
            }
            ControlRow {
                title: i18n("Automatic brightness")
                description: root.panel.automatic_brightness === true ? i18n("On") : i18n("Off")
                iconName: "brightness-high"
                available: typeof root.panel.automatic_brightness === "boolean"
                busy: root.pending > 0
                action: Component {
                    PC.Switch {
                        checked: root.panel.automatic_brightness === true
                        Accessible.name: i18n("Automatic brightness")
                        onToggled: root.change(checked ? "brightnessOn" : "brightnessOff")
                    }
                }
            }
            ControlRow {
                title: i18n("Cover")
                iconName: "computer-laptop"
                available: Control.property(root.desktop, "org.freedesktop.UPower", "LidIsPresent") === true
                description: Control.property(root.desktop, "org.freedesktop.UPower", "LidIsClosed") === true
                    ? i18n("Closed") : i18n("Open")
            }
            ControlRow {
                title: i18n("USB data role")
                iconName: "usb-creator"
                description: root.role()
                available: root.canRequestRole()
                busy: root.pending > 0
                action: Component {
                    RowLayout {
                        PC.Button { text: i18n("Host"); onClicked: root.confirm("host") }
                        PC.Button { text: i18n("Device"); onClicked: root.confirm("device") }
                    }
                }
            }
            ControlRow {
                title: i18n("USB serial at startup")
                iconName: "network-wired"
                description: typeof root.usb.cdc_saved_policy === "string" ? root.usb.cdc_saved_policy : i18n("State unavailable")
                available: root.usb.schema === "org.uke.control.usb.v1"
                busy: root.pending > 0
                action: Component {
                    RowLayout {
                        PC.Button { text: i18n("Enable"); onClicked: root.confirm("cdcOn") }
                        PC.Button { text: i18n("Disable"); onClicked: root.confirm("cdcOff") }
                    }
                }
            }
            ControlRow {
                title: i18n("Flashlight")
                iconName: "flashlight-off"
                description: Number.isInteger(root.lights.flashlight?.brightness_percent)
                    ? i18n("Requested: %1%; applied step: %2%", root.lights.flashlight?.requested_percent ?? 100, root.lights.flashlight.brightness_percent)
                    : i18n("Applied state unavailable")
                available: root.lights.flashlight?.available === true && typeof root.lights.flashlight?.on === "boolean"
                busy: root.pending > 0
                action: Component {
                    RowLayout {
                        PC.Switch {
                            checked: root.lights.flashlight?.on === true
                            Accessible.name: i18n("Flashlight")
                            onToggled: root.change(checked ? "flashlightOn" : "flashlightOff")
                        }
                        PC.ToolButton {
                            icon.name: "configure"
                            Accessible.name: i18n("Flashlight brightness")
                            onClicked: lightSettings.configure()
                        }
                    }
                }
            }
            ControlRow {
                title: i18n("RGB color and brightness")
                iconName: "color-management"
                description: root.rgb.available === true && Array.isArray(root.rgb.requested_color_rgb)
                    ? i18n("Requested: %1 %2 %3 at %4%", root.rgb.requested_color_rgb[0], root.rgb.requested_color_rgb[1], root.rgb.requested_color_rgb[2], root.rgb.requested_percent)
                    : i18n("State unavailable")
                available: root.rgb.available === true && root.rgb.kernel_io_error === 0
                busy: root.pending > 0
                action: Component {
                    PC.Button { text: i18n("Configure"); onClicked: rgbSettings.configure() }
                }
            }
            ControlRow {
                title: i18n("RGB color test")
                iconName: "color-management"
                description: root.rgb.mode === "test" ? i18n("Running the 10-second color transition")
                    : i18n("A 10-second transition through the color wheel")
                available: root.rgb.available === true && root.rgb.kernel_io_error === 0
                busy: root.pending > 0 || root.rgb.mode === "test" || root.rgb.mode === "front-camera"
                action: Component {
                    PC.Button { text: i18n("Test colors"); onClicked: root.change("rgbTest") }
                }
            }
            PC.Label {
                Layout.fillWidth: true
                Layout.margins: Kirigami.Units.largeSpacing
                wrapMode: Text.WordWrap
                text: i18n("Unsupported controls remain unavailable. Refresh reads the current state.")
            }
        }
        QQC2.Dialog {
            id: lightSettings
            function configure() {
                lightBrightness.value = root.lights.flashlight.requested_percent
                open()
            }
            anchors.centerIn: parent
            width: Math.min(parent.width, Kirigami.Units.gridUnit * 24)
            modal: true
            title: i18n("Flashlight brightness")
            standardButtons: QQC2.Dialog.Ok | QQC2.Dialog.Cancel
            contentItem: ColumnLayout {
                PC.Label { text: i18n("Brightness: %1%", Math.round(lightBrightness.value)) }
                QQC2.SpinBox {
                    id: lightBrightness
                    Layout.fillWidth: true
                    from: 1
                    to: 100
                    stepSize: 1
                    editable: true
                    Accessible.name: i18n("Brightness")
                }
                PC.Label {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    text: i18n("Every integer from 1 to 100 is accepted. Hardware applies 50% for requests 1–50 and 100% for 51–100. Maximum 20 mA. Turns off before sleep.")
                }
            }
            onAccepted: root.change("flashlightSet", [Math.round(lightBrightness.value)])
        }
        QQC2.Dialog {
            id: rgbSettings
            function configure() {
                redValue.value = root.rgb.requested_color_rgb[0]
                greenValue.value = root.rgb.requested_color_rgb[1]
                blueValue.value = root.rgb.requested_color_rgb[2]
                rgbBrightness.value = root.rgb.requested_percent
                open()
            }
            anchors.centerIn: parent
            width: Math.min(parent.width, Kirigami.Units.gridUnit * 24)
            modal: true
            title: i18n("RGB color and brightness")
            standardButtons: QQC2.Dialog.Ok | QQC2.Dialog.Cancel
            contentItem: ColumnLayout {
                PC.Label { text: i18n("Red") }
                QQC2.SpinBox { id: redValue; from: 0; to: 255; editable: true; Layout.fillWidth: true; Accessible.name: i18n("Red") }
                PC.Label { text: i18n("Green") }
                QQC2.SpinBox { id: greenValue; from: 0; to: 255; editable: true; Layout.fillWidth: true; Accessible.name: i18n("Green") }
                PC.Label { text: i18n("Blue") }
                QQC2.SpinBox { id: blueValue; from: 0; to: 255; editable: true; Layout.fillWidth: true; Accessible.name: i18n("Blue") }
                PC.Label { text: i18n("Brightness (%)") }
                QQC2.SpinBox { id: rgbBrightness; from: 0; to: 100; editable: true; Layout.fillWidth: true; Accessible.name: i18n("RGB brightness") }
                PC.Label {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    text: i18n("Brightness follows the device limit. Zero restores automatic colors. Camera and low-battery warnings take priority over a manual color.")
                }
                PC.Button { text: i18n("Restore automatic colors"); onClicked: { rgbSettings.close(); root.change("rgbAuto") } }
            }
            onAccepted: root.change("rgbSet", [redValue.value, greenValue.value, blueValue.value, rgbBrightness.value])
        }
        QQC2.Dialog {
            id: confirmation
            anchors.centerIn: parent
            width: Kirigami.Units.gridUnit * 22
            modal: true
            title: i18n("Change USB configuration?")
            standardButtons: QQC2.Dialog.Ok | QQC2.Dialog.Cancel
            contentItem: PC.Label {
                text: i18n("This can disconnect USB accessories or a remote session. Serial startup settings are saved. Administrator authorization is required.")
                wrapMode: Text.WordWrap
            }
            onAccepted: root.change(root.requestedAction)
        }
    }
}
