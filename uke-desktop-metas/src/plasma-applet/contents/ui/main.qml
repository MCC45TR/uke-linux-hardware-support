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
    readonly property var panel: requestState.snapshots.plasma || ({})
    readonly property var desktop: requestState.snapshots.desktop || ({})
    readonly property var usb: requestState.snapshots.usb || ({})
    readonly property var lights: requestState.snapshots.lights || ({})
    readonly property var rgb: requestState.snapshots.rgb || ({})
    readonly property string errorText: requestState.errorText
    readonly property int pending: requestState.pending
    property bool fixtureMode: false
    property string requestedAction: ""

    function execute(action, values) {
        requestState.execute(action, values)
    }
    function refresh() {
        requestState.refresh()
    }
    function change(action, values) {
        execute(action, values)
    }
    function finish(source, data) {
        requestState.finish(source, data, false)
    }
    function busy(domain) { return requestState.busy(domain) }
    function availability(action, supported) { return requestState.availability(action, supported) }
    function coverState() {
        const upower = Control.property(desktop, "org.freedesktop.UPower", "LidIsClosed")
        const logind = Control.property(desktop, "org.freedesktop.login1", "LidClosed")
        if (typeof upower === "boolean" && typeof logind === "boolean" && upower !== logind) return i18n("Conflicting cover readings")
        const value = typeof upower === "boolean" ? upower : logind
        return value === true ? i18n("Closed") : value === false ? i18n("Open") : i18n("Cover position unknown")
    }
    RequestState {
        id: requestState
        readFailureText: i18n("Could not read tablet state. Refresh to try again.")
        writeFailureText: i18n("The change failed or authorization was cancelled.")
        timeoutText: i18n("The state query timed out. Refresh to try again.")
        onDispatch: function(source) { if (!root.fixtureMode) executable.connectSource(source) }
        onRelease: function(source) { if (!root.fixtureMode) executable.disconnectSource(source) }
        onRefreshRequested: root.refresh()
    }
    Timer {
        interval: 1000
        repeat: true
        running: root.pending > 0
        onTriggered: requestState.expire(Date.now())
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
    onExpandedChanged: { if (expanded) refresh() }

    Timer {
        interval: 2000
        repeat: true
        running: root.expanded && (root.lights.flashlight?.on === true || root.rgb.mode === "test")
        onTriggered: {
            if (root.lights.flashlight?.on === true) root.execute("lights")
            if (root.rgb.mode === "test") root.execute("rgb")
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
                    onClicked: root.refresh()
                }
            }
            Kirigami.InlineMessage {
                Layout.fillWidth: true
                Layout.margins: Kirigami.Units.largeSpacing
                visible: root.fixtureMode
                text: i18n("Simulation: hardware commands are disabled.")
                type: Kirigami.MessageType.Information
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
                description: root.panel.rotation === true ? i18n("Always") : root.panel.rotation === false ? i18n("Off or tablet mode only") : i18n("State unknown")
                iconName: "object-rotate-right"
                available: typeof root.panel.rotation === "boolean"
                availabilityState: root.availability("plasma", available)
                busy: root.busy("display")
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
                description: root.panel.automatic_brightness === true ? i18n("On") : root.panel.automatic_brightness === false ? i18n("Off") : i18n("State unknown")
                iconName: "brightness-high"
                available: typeof root.panel.automatic_brightness === "boolean"
                availabilityState: root.availability("plasma", available)
                busy: root.busy("display")
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
                description: root.coverState()
                availabilityState: root.availability("desktop", available)
                busy: root.busy("desktop")
            }
            ControlRow {
                title: i18n("USB data role")
                iconName: "usb-creator"
                description: root.role()
                available: root.canRequestRole()
                availabilityState: root.availability("usb", available)
                busy: root.busy("usb")
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
                availabilityState: root.availability("usb", available)
                busy: root.busy("usb")
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
                availabilityState: root.availability("lights", available)
                busy: root.busy("lights")
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
                availabilityState: root.availability("rgb", available)
                busy: root.busy("rgb") || root.rgb.mode === "test" || root.rgb.mode === "front-camera"
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
                lightBrightness.value = root.lights.flashlight?.requested_percent ?? 100
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
                    text: i18n("Hardware has two lit steps: 1–99% requests use 50%; 100% uses 100%. Maximum 20 mA. No automatic timeout; turns off before sleep.")
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
