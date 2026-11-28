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
        visible: backend.simulation || backend.readOnly
        type: Kirigami.MessageType.Information
        text: backend.simulation ? i18n("Simulation — sample data only. Changes are disabled.") : i18n("Read-only session")
    }
    RowLayout {
        Layout.fillWidth: true
        Layout.maximumWidth: Kirigami.Units.gridUnit * 30
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
    TileGrid {
        objectName: "informationTiles"
        visible: settings.information.length > 0
        Layout.maximumWidth: settings.information.length > 1 && page.width >= minimumTileWidth * 2 + columnSpacing * 3
            ? Kirigami.Units.gridUnit * 62 : Kirigami.Units.gridUnit * 30
        Layout.alignment: Qt.AlignHCenter
        Layout.margins: Kirigami.Units.largeSpacing
        maximumColumns: 2
        minimumTileWidth: Kirigami.Units.gridUnit * 22
        Repeater {
            model: settings.information
            delegate: ColumnLayout {
                id: informationSection
                required property var modelData
                property bool expanded: false
                readonly property int previewRows: modelData.compact ? 2 : 6
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                Layout.alignment: Qt.AlignTop
                spacing: 0
                FormCard.FormHeader { title: modelData.title }
                FormCard.FormCard {
                    Layout.fillWidth: true
                    Repeater {
                        model: modelData.rows
                        delegate: ColumnLayout {
                            required property var modelData
                            required property int index
                            Layout.fillWidth: true
                            visible: informationSection.expanded || (!modelData.detail && index < informationSection.previewRows)
                            spacing: 0
                            SafeText { Layout.fillWidth: true; text: modelData.title; description: modelData.value }
                            Controls.ProgressBar {
                                Layout.fillWidth: true
                                Layout.leftMargin: Kirigami.Units.largeSpacing
                                Layout.rightMargin: Kirigami.Units.largeSpacing
                                Layout.bottomMargin: Kirigami.Units.largeSpacing
                                visible: modelData.progress >= 0
                                value: modelData.progress >= 0 ? modelData.progress : 0
                                Accessible.name: modelData.title
                            }
                        }
                    }
                    FormCard.FormButtonDelegate {
                        visible: informationSection.modelData.rows.length > informationSection.previewRows || informationSection.modelData.rows.some(r => r.detail)
                        text: informationSection.expanded ? i18n("Less information") : i18n("More information")
                        icon.name: themeIcons.names["view-list-details"]
                        Accessible.name: text
                        onClicked: informationSection.expanded = !informationSection.expanded
                    }
                    SafeText { visible: modelData.stale || false; text: i18n("Previous information — refresh failed"); description: modelData.stamp || "" }
                }
            }
        }
    }
    Loader {
        Layout.fillWidth: true
        active: page.topicId === "battery"
        sourceComponent: Component { BatteryHistoryView {} }
    }
    FormCard.FormCard {
        FormCard.FormButtonDelegate {
            text: i18n("Technical tools")
            description: i18n("Additional observations and expert actions")
            icon.name: themeIcons.names["utilities-terminal"]
            Accessible.name: text
            onClicked: page.appWindow.openCatalog(settings.topic.area)
        }
    }
}
