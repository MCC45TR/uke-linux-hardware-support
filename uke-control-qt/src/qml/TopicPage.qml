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
    function edit(id) {
        if (backend.openAction(id)) {
            editAction = id
            if ((backend.guide.parameters || []).length === 0) backend.submit()
        }
    }
    function showSinglePreference() {
        if (page.appWindow.pageStack.currentItem !== page || settings.topic.id !== topicId || backend.readOnly || settings.topic.canApply === false) return
        const choices = settings.topic.changes || []
        if (choices.length === 1 && !editing) edit(choices[0].id)
    }
    Component.onCompleted: Qt.callLater(showSinglePreference)
    Connections { target: page.settings; function onChanged() { page.showSinglePreference() } }
    title: settings.topic.title || ""
    actions: [
        Kirigami.Action { text: i18n("Refresh"); icon.name: themeIcons.names["view-refresh"]; enabled: !settings.busy; onTriggered: { backend.cancelReview(); backend.cancelRead(); settings.refresh() } },
        Kirigami.Action { text: i18n("Stop"); icon.name: themeIcons.names["media-playback-stop"]; enabled: settings.busy || settings.history.busy || false; onTriggered: settings.stop() }
    ]
    Kirigami.InlineMessage {
        Layout.fillWidth: true
        visible: backend.simulation || backend.readOnly
        type: Kirigami.MessageType.Information
        text: backend.simulation ? i18n("Simulation — sample data only. Changes are disabled.") : i18n("Read-only session")
    }
    RowLayout {
        Layout.fillWidth: true
        Layout.leftMargin: Kirigami.Units.largeSpacing
        Layout.rightMargin: Kirigami.Units.largeSpacing
        Controls.BusyIndicator { visible: settings.busy; running: visible }
        Controls.Label {
            Layout.fillWidth: true
            text: settings.busy ? i18n("Reading…") : settings.updated.length ? i18n("Read at %1", settings.updated) : settings.topic.description || ""
            wrapMode: Text.WordWrap
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
        active: Object.keys(settings.bootReport).length > 0
        sourceComponent: Component { UpdateStatusView { report: settings.bootReport } }
    }
    Repeater {
        model: settings.information
        delegate: ColumnLayout {
            id: informationSection
            required property var modelData
            property bool expanded: false
            Layout.fillWidth: true
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
                        visible: informationSection.expanded || (!modelData.detail && index < 6)
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
                    visible: informationSection.modelData.rows.length > 6 || informationSection.modelData.rows.some(r => r.detail)
                    text: informationSection.expanded ? i18n("Less information") : i18n("More information")
                    icon.name: themeIcons.names["view-list-details"]
                    Accessible.name: text
                    onClicked: informationSection.expanded = !informationSection.expanded
                }
                SafeText { visible: modelData.stale; text: i18n("Previous information — refresh failed"); description: modelData.stamp }
            }
        }
    }
    Loader {
        Layout.fillWidth: true
        active: page.topicId === "battery"
        sourceComponent: Component { BatteryHistoryView {} }
    }
    FormCard.FormHeader { visible: (settings.topic.changes || []).length > 1; title: i18n("Preferences and actions") }
    Kirigami.InlineMessage {
        Layout.fillWidth: true
        visible: settings.topic.canApply === false
        type: Kirigami.MessageType.Information
        text: settings.topic.blockedReason || ""
    }
    FormCard.FormCard {
        visible: (settings.topic.changes || []).length > 1 || ((settings.topic.changes || []).length === 1 && !page.editing)
        Repeater {
            model: settings.topic.changes || []
            delegate: FormCard.FormButtonDelegate {
                required property var modelData
                objectName: "topicAction_" + modelData.id
                text: modelData.title
                icon.name: themeIcons.names["configure"]
                Accessible.name: text
                enabled: settings.topic.canApply !== false
                onClicked: { settings.stop(); page.edit(modelData.id) }
            }
        }
    }
    Loader {
        Layout.fillWidth: true
        active: page.editing
        sourceComponent: Component { PreferenceEditor { pageWidth: page.width } }
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
