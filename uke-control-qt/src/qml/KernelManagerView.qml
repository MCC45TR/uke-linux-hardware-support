// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard
ColumnLayout {
    id: view
    required property string mode
    property var appWindow
    property bool currentPage: false
    readonly property var manager: backend.settings.manager
    property int seconds: 120
    property bool optionsExpanded: false
    Layout.fillWidth: true
    spacing: 0
    FormCard.FormHeader { title: i18n("Senemos Kernel Manager") }
    FormCard.FormCard {
        visible: view.mode === "manager-overview"
        Layout.fillWidth: true
        Repeater {
            model: [{id:"cpu",title:i18n("CPU"),icon:"cpu"},{id:"gpu",title:i18n("GPU"),icon:"video-display"},{id:"memory",title:i18n("Memory and swap"),icon:"cpu"},{id:"storage",title:i18n("Storage"),icon:"drive-harddisk"},{id:"thermal",title:i18n("Thermal and sleep"),icon:"preferences-system-power-management"},{id:"profiles",title:i18n("Application profiles"),icon:"configure"},{id:"diagnostics",title:i18n("Diagnostics"),icon:"utilities-system-monitor"}]
            delegate: FormCard.FormButtonDelegate { required property var modelData; text: modelData.title; icon.name: themeIcons.names[modelData.icon]; onClicked: if (view.appWindow) view.appWindow.openTopic(modelData.id) }
        }
    }
    FormCard.FormCard {
        Layout.fillWidth: true
        SafeText { visible: view.optionsExpanded; text: i18n("Session observations"); description: i18n("Graphs contain observations from this session only. Collection pauses when leaving this page or switching away from the application.") }
        FormCard.FormSwitchDelegate {
            objectName: "kernelMonitoring"
            text: i18n("Live monitoring")
            checked: view.manager.active
            onToggled: { if (view.currentPage) { if (checked) view.manager.start(); else view.manager.pause() } }
        }
        FormCard.FormComboBoxDelegate {
            visible: view.optionsExpanded
            text: i18n("Read interval")
            model: [{title:i18n("1 second"),value:1},{title:i18n("2 seconds"),value:2},{title:i18n("5 seconds"),value:5}]
            textRole: "title"
            currentIndex: view.manager.interval === 1 ? 0 : view.manager.interval === 2 ? 1 : 2
            onActivated: index => view.manager.setInterval(model[index].value)
        }
        FormCard.FormComboBoxDelegate {
            visible: view.optionsExpanded
            text: i18n("Graph window")
            model: [{title:i18n("1 minute"),value:60},{title:i18n("2 minutes"),value:120},{title:i18n("10 minutes"),value:600}]
            textRole: "title"
            currentIndex: view.seconds === 60 ? 0 : view.seconds === 120 ? 1 : 2
            onActivated: index => view.seconds = model[index].value
        }
        FormCard.FormButtonDelegate { text: i18n("Monitoring options"); icon.name: themeIcons.names["configure"]; onClicked: view.optionsExpanded = !view.optionsExpanded }
        FormCard.FormButtonDelegate { visible: !view.manager.active; text: i18n("Read once"); icon.name: themeIcons.names["view-refresh"]; enabled: !view.manager.busy; onClicked: view.manager.refresh() }
    }
    Kirigami.InlineMessage { Layout.fillWidth: true; visible: view.manager.message.length > 0; text: view.manager.message; type: Kirigami.MessageType.Information }
    Loader {
        Layout.fillWidth: true
        active: view.mode === "cpu" || view.mode === "gpu"
        sourceComponent: Component { PerformanceView { manager: view.manager; mode: view.mode; seconds: view.seconds } }
    }
    Loader {
        Layout.fillWidth: true
        active: view.mode === "profiles"
        sourceComponent: Component { ApplicationProfiles { manager: view.manager } }
    }
    Loader {
        Layout.fillWidth: true
        active: view.mode !== "sensors"
        sourceComponent: Component { ResourceView { manager: view.manager; mode: view.mode; seconds: view.seconds } }
    }
    Loader {
        Layout.fillWidth: true
        active: view.mode === "sensors"
        sourceComponent: Component { SensorView { manager: view.manager; seconds: view.seconds; currentPage: view.currentPage } }
    }
    FormCard.FormCardDialog {
        id: review
        objectName: "frequencyReview"
        title: i18n("Review kernel resource change")
        width: Math.min(parent.width - Kirigami.Units.largeSpacing * 2, Kirigami.Units.gridUnit * 30)
        height: Math.min(implicitHeight, parent.height - Kirigami.Units.largeSpacing * 2)
        standardButtons: view.manager.busy ? Controls.Dialog.NoButton : Controls.Dialog.Cancel
        closePolicy: view.manager.busy ? Controls.Popup.NoAutoClose : Controls.Popup.CloseOnEscape
        onRejected: view.manager.dismissReview()
        SafeText { text: view.manager.review.id || ""; description: view.manager.review.field === "maximum" ? i18n("Maximum frequency") : view.manager.review.field === "minimum" ? i18n("Minimum frequency") : view.manager.review.field === "governor" ? i18n("Governor") : view.manager.review.field || "" }
        SafeText { text: i18n("Current value"); description: view.manager.review.before || "" }
        SafeText { text: i18n("Requested value"); description: view.manager.review.after || "" }
        SafeText { text: i18n("Scope of the change"); description: view.manager.review.kind === "application" ? i18n("This preset affects only the selected application invocation. Existing limits are replaced with the reviewed preset. The current unit and resulting limits are checked before reporting success.") : i18n("One runtime setting is changed after administrator authorization and checked by reading it back. Changes are not saved for the next boot. Memory and storage controls affect the whole system. Desktop power management can subsequently replace frequency settings.") }
        SafeText { visible: view.manager.review.blocked || false; text: i18n("Action disabled"); description: i18n("Simulation and read-only sessions cannot apply frequency changes.") }
        FormCard.FormButtonDelegate {
            objectName: "applyFrequency"
            text: i18n("Apply and verify")
            icon.name: themeIcons.names["dialog-ok"]
            enabled: !(view.manager.review.blocked || false) && !view.manager.busy
            onClicked: view.manager.apply(view.manager.review.token || "")
        }
    }
    onCurrentPageChanged: if (!currentPage) review.close()
    Connections { target: view.manager; function onReviewChanged() { if (view.currentPage && Object.keys(view.manager.review).length) review.open(); else review.close() } }
}
