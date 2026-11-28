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
    readonly property bool showConnectionsFirst: topicId === "usb" || topicId === "nfc"
    readonly property real readingColumnWidth: Kirigami.Units.gridUnit * (information.length > 1 || topicId === "kernel" ? 48 : 30)
    property string editAction: ""
    readonly property bool editing: editAction.length > 0 && backend.guide.id === editAction
    readonly property var topicActions: (settings.topic.changes || []).filter(action => action.id !== "usb.role")
    function edit(id, submitWithoutOptions = false) {
        if (backend.openAction(id)) {
            editAction = id
            if (submitWithoutOptions && (backend.guide.parameters || []).length === 0) backend.submit()
        }
    }
    function showSinglePreference() {
        if (page.appWindow.changingPage || page.appWindow.pageStack.currentItem !== page || settings.topic.id !== topicId || backend.readOnly || settings.topic.canApply === false) return
        const choices = settings.topic.changes || []
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
        visible: settings.notice.length > 0
        type: Kirigami.MessageType.Warning
        text: settings.notice
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
        active: page.topicId === "usb"
        sourceComponent: Component { UsbRoleView { topicPage: page } }
    }
    Loader {
        Layout.fillWidth: true
        active: page.topicId === "nfc"
        sourceComponent: Component { NfcView {} }
    }
    Component {
        id: informationPanel
        ColumnLayout {
            Layout.fillWidth: true
            Kirigami.Heading {
                visible: page.topicActions.length > 0
                Layout.fillWidth: true
                Layout.maximumWidth: readings.width
                Layout.alignment: Qt.AlignHCenter
                level: 3
                text: i18n("Reported information")
            }
            InformationGrid {
                id: readings
                objectName: "informationTiles"
                sections: page.information
            }
        }
    }
    Loader {
        Layout.fillWidth: true
        active: !page.showConnectionsFirst && page.information.length > 0
        sourceComponent: informationPanel
    }
    FormCard.FormHeader {
        visible: actionList.visible
        title: i18n("Settings and actions")
    }
    Kirigami.InlineMessage {
        Layout.fillWidth: true
        visible: !settings.busy && settings.topic.canApply === false
        type: Kirigami.MessageType.Information
        text: settings.topic.blockedReason || ""
    }
    FormCard.FormCard {
        id: actionList
        visible: page.topicActions.length > 1 || (page.topicActions.length === 1 && !page.editing)
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
        FormCard.FormButtonDelegate {
            text: i18n("Technical tools")
            description: i18n("Additional observations and expert actions")
            icon.name: themeIcons.names["utilities-terminal"]
            Accessible.name: text
            onClicked: page.appWindow.openCatalog(settings.topic.area)
        }
    }
}
