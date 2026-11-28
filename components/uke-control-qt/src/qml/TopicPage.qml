// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

FormCard.FormCardPage {
    id: page
    objectName: "topicPage"
    required property var appWindow
    required property string topicId
    readonly property var settings: backend.settings
    readonly property var information: settings.information
    readonly property bool isCharging: topicId === "charging"
    readonly property bool showConnectionsFirst: topicId === "usb" || topicId === "nfc"
    readonly property real readingColumnWidth: Kirigami.Units.gridUnit * 64
    readonly property bool dedicatedActions: isCharging || topicId === "updates" || topicId === "kernel"
    property string editAction: ""
    readonly property bool editing: editAction.length > 0 && backend.guide.id === editAction
    readonly property var topicActions: (settings.topic.changes || []).filter(action =>
        !["usb.role", "display.lanes", "display.compression-set"].includes(action.id))
    function edit(id, submitWithoutOptions = false) {
        if (backend.openAction(id)) {
            editAction = id
            if (submitWithoutOptions && (backend.guide.parameters || []).length === 0) backend.submit()
        }
    }
    function showSinglePreference() {
        if (page.appWindow.changingPage || page.appWindow.pageStack.currentItem !== page || settings.topic.id !== topicId || backend.readOnly || settings.topic.canApply === false || dedicatedActions || topicId === "external") return
        const choices = page.topicActions
        if (choices.length === 1 && !editing) edit(choices[0].id)
    }
    Component.onCompleted: Qt.callLater(showSinglePreference)
    Connections { target: page.settings; function onChanged() { page.showSinglePreference() } }
    title: settings.topic.title || ""
    actions: [
        Kirigami.Action { text: i18n("Refresh"); icon.name: themeIcons.names["view-refresh"]; enabled: !settings.busy; onTriggered: { backend.cancelReview(); backend.cancelRead(); settings.refresh() } },
        Kirigami.Action { text: i18n("Stop"); icon.name: themeIcons.names["media-playback-stop"]; enabled: settings.busy || settings.history.busy || settings.manager.active || settings.manager.busy || false; onTriggered: settings.stop() }
    ]
    Kirigami.InlineMessage {
        Layout.fillWidth: true
        Layout.maximumWidth: page.readingColumnWidth
        Layout.alignment: Qt.AlignHCenter
        Layout.margins: Kirigami.Units.largeSpacing
        visible: backend.simulation || backend.readOnly
        type: Kirigami.MessageType.Information
        text: backend.simulation ? i18n("Simulation — sample data only. Changes are disabled.") : i18n("Read-only session")
    }
    RowLayout {
        Layout.fillWidth: true
        Layout.maximumWidth: page.readingColumnWidth
        Layout.alignment: Qt.AlignHCenter
        Layout.leftMargin: Kirigami.Units.largeSpacing
        Layout.rightMargin: Kirigami.Units.largeSpacing
        Layout.topMargin: Kirigami.Units.mediumSpacing
        Layout.bottomMargin: Kirigami.Units.mediumSpacing
        Controls.BusyIndicator { visible: settings.busy; running: visible }
        ColumnLayout {
            Layout.fillWidth: true
            spacing: Kirigami.Units.smallSpacing
            Controls.Label {
                Layout.fillWidth: true
                text: page.settings.topic.description || ""
                wrapMode: Text.Wrap
            }
            Controls.Label {
                Layout.fillWidth: true
                visible: text.length > 0
                text: page.settings.busy ? i18n("Reading…") : page.settings.updated.length ? i18n("Read at %1", page.settings.updated) : ""
                font: Kirigami.Theme.smallFont
                wrapMode: Text.Wrap
            }
        }
    }
    Kirigami.InlineMessage {
        Layout.fillWidth: true
        Layout.maximumWidth: page.isCharging ? page.readingColumnWidth : Number.POSITIVE_INFINITY
        Layout.alignment: Qt.AlignHCenter
        Layout.leftMargin: page.isCharging ? Kirigami.Units.largeSpacing : 0
        Layout.rightMargin: Layout.leftMargin
        visible: settings.notice.length > 0
        type: Kirigami.MessageType.Warning
        text: settings.notice
    }
    Loader {
        Layout.fillWidth: true
        active: page.topicId === "storage"
        sourceComponent: Component { StoragePartitions { settings: page.settings } }
    }
    Loader {
        Layout.fillWidth: true
        active: ["cpu", "gpu", "sensors", "manager-overview", "memory", "storage", "thermal", "profiles", "diagnostics"].includes(page.topicId)
        sourceComponent: Component { KernelManagerView { mode: page.topicId; appWindow: page.appWindow; currentPage: page.appWindow.pageStack.currentItem === page && settings.topic.id === page.topicId } }
    }
    Loader {
        Layout.fillWidth: true
        active: Object.keys(settings.bootReport).length > 0
        sourceComponent: Component { UpdateStatusView { report: settings.bootReport } }
    }
    Loader {
        Layout.fillWidth: true
        Layout.maximumWidth: page.readingColumnWidth
        Layout.alignment: Qt.AlignHCenter
        Layout.margins: Kirigami.Units.largeSpacing
        active: page.topicId === "kernel" && Object.keys(settings.bootReport).length > 0
        visible: active
        sourceComponent: Component { BootSelection { settings: page.settings } }
    }
    Loader {
        Layout.fillWidth: true
        Layout.maximumWidth: page.readingColumnWidth
        Layout.alignment: Qt.AlignHCenter
        Layout.margins: Kirigami.Units.largeSpacing
        active: page.topicId === "updates"
        visible: active
        sourceComponent: Component { SystemUpdatePanel { settings: page.settings; appWindow: page.appWindow } }
    }
    Loader {
        Layout.fillWidth: true
        active: page.topicId === "usb"
        sourceComponent: Component { UsbRoleView { topicPage: page } }
    }
    Loader {
        Layout.fillWidth: true
        active: page.topicId === "nfc"
        sourceComponent: Component { NfcView {} }
    }
    Loader {
        Layout.fillWidth: true
        active: page.topicId === "flashlight"
        sourceComponent: Component { FlashlightPanel { settings: page.settings } }
    }
    Loader {
        Layout.fillWidth: true
        active: page.topicId === "external"
        sourceComponent: Component { DisplayLinkPanel { settings: page.settings; appWindow: page.appWindow } }
    }
    Loader {
        Layout.fillWidth: true
        Layout.maximumWidth: page.readingColumnWidth
        Layout.alignment: Qt.AlignHCenter
        Layout.leftMargin: Kirigami.Units.largeSpacing
        Layout.rightMargin: Kirigami.Units.largeSpacing
        active: page.isCharging
        visible: active
        sourceComponent: Component { ChargingPanel { settings: page.settings; information: page.information } }
    }
    Component {
        id: informationPanel
        ColumnLayout {
            Layout.fillWidth: true
            Kirigami.Heading {
                visible: page.topicActions.length > 0 && !page.dedicatedActions
                Layout.fillWidth: true
                Layout.maximumWidth: readings.width
                Layout.alignment: Qt.AlignHCenter
                level: 3
                text: i18n("Reported information")
            }
            InformationGrid {
                id: readings
                objectName: "informationTiles"
                Layout.maximumWidth: page.readingColumnWidth
                sections: page.information
            }
        }
    }
    Loader {
        Layout.fillWidth: true
        active: !page.isCharging && !page.showConnectionsFirst && page.topicId !== "flashlight" && page.topicId !== "updates" && page.information.length > 0
        sourceComponent: informationPanel
    }
    FormCard.FormHeader {
        visible: actionList.visible
        title: i18n("Settings and actions")
    }
    Kirigami.InlineMessage {
        Layout.fillWidth: true
        visible: !page.isCharging && !settings.busy && settings.topic.canApply === false
        type: Kirigami.MessageType.Information
        text: settings.topic.blockedReason || ""
    }
    FormCard.FormCard {
        id: actionList
        visible: !page.dedicatedActions && (page.topicActions.length > 1 || (page.topicActions.length === 1 && !page.editing))
        Repeater {
            model: page.topicActions
            delegate: FormCard.FormButtonDelegate {
                required property var modelData
                objectName: "topicAction_" + modelData.id
                text: modelData.title
                highlighted: page.editing && page.editAction === modelData.id
                enabled: settings.topic.canApply !== false && (!backend.readOnly || modelData.read)
                Accessible.name: text
                onClicked: { settings.stop(); page.edit(modelData.id, true) }
            }
        }
    }
    Loader {
        Layout.fillWidth: true
        active: page.editing && page.appWindow.pageStack.currentItem === page
        sourceComponent: Component { PreferenceEditor {} }
    }
    Loader {
        Layout.fillWidth: true
        active: page.showConnectionsFirst && page.information.length > 0
        sourceComponent: informationPanel
    }
    Loader {
        Layout.fillWidth: true
        active: page.topicId === "battery"
        sourceComponent: Component { BatteryHistoryView {} }
    }
    FormCard.FormCard {
        maximumWidth: page.readingColumnWidth
        Repeater {
            model: page.isCharging ? page.topicActions : []
            delegate: FormCard.FormButtonDelegate {
                required property var modelData
                text: modelData.id === "charge.mode" ? i18n("Other charging policies") : i18n("Stop threshold only")
                enabled: settings.topic.canApply !== false && !backend.readOnly
                onClicked: page.appWindow.openGuide(modelData.id)
            }
        }
        FormCard.FormButtonDelegate {
            text: i18n("Technical tools")
            description: i18n("Additional observations and expert actions")
            icon.name: themeIcons.names["utilities-terminal"]
            Accessible.name: text
            onClicked: page.appWindow.openCatalog(settings.topic.area)
        }
    }
}
