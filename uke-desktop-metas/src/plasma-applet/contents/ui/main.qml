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
import "Control.js" as Control

PlasmoidItem {
    id: root
    property var panel: ({})
    property var desktop: ({})
    property var usb: ({})
    property var lights: ({})
    property var rgb: ({})
    property var storage: ({})
    property var hub: ({})
    property var requests: ({})
    property string errorText: ""
    property int serial: 0
    property int pending: 0
    property bool maintenancePending: false
    property string requestedAction: ""

    function execute(action, values) {
        const command = Control.command(action, values)
        if (!command) return
        const source = "UKE_TRAY_REQUEST=" + (++serial) + " " + command
        requests[source] = action
        if (action === "maintenance") maintenancePending = true
        else ++pending
        executable.connectSource(source)
    }
    function refresh() {
        if (pending > 0) return
        execute("plasma")
        execute("desktop")
        execute("usb")
        execute("lights")
        execute("rgb")
        execute("storage")
        execute("hub")
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
        if (action === "maintenance") {
            maintenancePending = false
            if (data["exit code"] !== 0) errorText = i18n("Maintenance terminal exited with an error. Check the terminal result and that Konsole is installed.")
            return
        }
        --pending
        const query = action === "plasma" || action === "desktop" || action === "usb" || action === "lights" || action === "rgb" || action === "storage" || action === "hub"
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
            else if (action === "hub") hub = value
            else if (action === "storage") storage = value
            else rgb = value
        } else {
            if (data["exit code"] !== 0)
                errorText = i18n("The change failed or authorization was cancelled.")
            refresh()
        }
    }
    function storageReason(device) {
        switch (device.reason) {
        case "device_identity_unverified": return i18n("Device identity unverified")
        case "whole_disk_is_not_a_root_filesystem": return i18n("Whole disk; select a root filesystem partition")
        case "mount_inventory_unknown": return i18n("Mount state unknown; inspection blocked")
        case "mounted_device": return i18n("Mounted / in use")
        case "swap_dependencies_unknown": return i18n("Active swap or unknown swap dependencies")
        case "holder_inventory_unknown": return i18n("Device dependencies unknown")
        case "device_has_holders": return i18n("In use by another block device")
        case "read_only_or_unknown": return i18n("Read-only state or write permission unknown")
        case "filesystem_inspection_required": return i18n("Filesystem inspection required")
        default: return i18n("Unknown prerequisite; operation unavailable")
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
    toolTipMainText: i18n("Uke Control Center")
    toolTipSubText: i18n("Hardware, lights and maintenance")
    onExpandedChanged: { if (expanded) refresh() }

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

    Timer {
        interval: 10000
        repeat: true
        running: root.expanded
        onTriggered: { if (root.pending === 0) root.execute("hub") }
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
            Kirigami.Heading {
                Layout.margins: Kirigami.Units.largeSpacing
                text: i18n("Live hardware")
                level: 3
            }
            PC.Label {
                Layout.fillWidth: true
                Layout.margins: Kirigami.Units.largeSpacing
                text: root.hub.items ? i18n("Kernel-reported observations; physical operation is not inferred.") : i18n("Hardware state unknown. Refresh to retry.")
                wrapMode: Text.WordWrap
            }
            Repeater {
                model: root.hub.items || []
                delegate: RowLayout {
                    required property var modelData
                    Layout.fillWidth: true
                    Layout.leftMargin: Kirigami.Units.largeSpacing
                    Layout.rightMargin: Kirigami.Units.largeSpacing
                    PC.Label { text: modelData.label; textFormat: Text.PlainText; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                    PC.Label {
                        Layout.maximumWidth: Kirigami.Units.gridUnit * 15
                        text: modelData.state === "available" ? modelData.value + " " + modelData.unit : modelData.state === "unsupported" ? i18n("Not exported by kernel") : modelData.state === "error" ? i18n("Read error") : i18n("Unknown")
                        textFormat: Text.PlainText
                        wrapMode: Text.WordWrap
                        QQC2.ToolTip.text: modelData.reason || i18n("Live kernel sample")
                    }
                }
            }
            PC.Button {
                Layout.margins: Kirigami.Units.largeSpacing
                text: i18n("Open maintenance terminal…")
                icon.name: "utilities-terminal"
                enabled: root.pending === 0 && !root.maintenancePending
                onClicked: root.change("maintenance")
                Accessible.description: i18n("Administrator authorization and explicit target trust are required for a read-only chroot session.")
            }
            Kirigami.Heading {
                Layout.margins: Kirigami.Units.largeSpacing
                text: i18n("Storage and maintenance")
                level: 3
            }
            PC.Label {
                Layout.fillWidth: true
                Layout.margins: Kirigami.Units.largeSpacing
                text: root.storage.devices ? i18n("%1 block devices. Read-only inventory. Open the maintenance terminal for authorized root inspection.", root.storage.devices.length) : i18n("Storage state unknown. Refresh to retry.")
                wrapMode: Text.WordWrap
                textFormat: Text.PlainText
            }
            Repeater {
                model: root.storage.devices || []
                delegate: PC.Label {
                    required property var modelData
                    Layout.fillWidth: true
                    Layout.leftMargin: Kirigami.Units.largeSpacing
                    Layout.rightMargin: Kirigami.Units.largeSpacing
                    text: modelData.path + " (" + modelData.major_minor + ") — " + root.storageReason(modelData)
                    wrapMode: Text.WordWrap
                    textFormat: Text.PlainText
                }
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
                description: i18n("Requested: %1%; applied step: %2%", root.lights.flashlight?.requested_percent ?? 100, root.lights.flashlight?.brightness_percent ?? 0)
                available: root.lights.flashlight?.available === true
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
                QQC2.Slider {
                    id: lightBrightness
                    Layout.fillWidth: true
                    from: 1
                    to: 100
                    stepSize: 1
                    snapMode: QQC2.Slider.SnapAlways
                    Accessible.name: i18n("Brightness")
                }
                PC.Label {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    text: i18n("Hardware has two lit steps: 1–99% requests use 50%; 100% uses 100%. Maximum 20 mA. Automatic off after 60 seconds; turns off before sleep.")
                }
            }
            onAccepted: root.change("flashlightSet", [Math.round(lightBrightness.value)])
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
