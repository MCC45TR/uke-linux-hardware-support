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
    readonly property bool dedicatedActions: isCharging || ["updates", "kernel", "display", "led"].includes(topicId)
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
        if (page.appWindow.changingPage || page.appWindow.pageStack.currentItem !== page || settings.topic.id !== topicId || backend.readOnly || settings.topic.canApply === false || dedicatedActions) return
        const choices = page.topicActions
        if (choices.length === 1 && !editing) edit(choices[0].id)
    }
    Component.onCompleted: Qt.callLater(showSinglePreference)
    Connections { target: page.settings; function onChanged() { page.showSinglePreference() } }
    component PagePanel: Loader {
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        Layout.maximumWidth: page.readingColumnWidth
        Layout.alignment: Qt.AlignHCenter
        Layout.margins: Kirigami.Units.largeSpacing
        visible: active
    }
    title: settings.topic.title || ""
    Kirigami.InlineMessage {
        Layout.fillWidth: true
        Layout.maximumWidth: page.readingColumnWidth
        Layout.alignment: Qt.AlignHCenter
        Layout.margins: Kirigami.Units.largeSpacing
        visible: backend.simulation || backend.readOnly
        type: Kirigami.MessageType.Information
        text: backend.simulation ? i18n("Simulation — sample data only. Changes are disabled.") : i18n("Read-only session")
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
    PagePanel {
        active: page.topicId === "storage"
        sourceComponent: Component { StoragePartitions { settings: page.settings } }
    }
    PagePanel {
        active: ["cpu", "gpu", "sensors", "manager-overview", "memory", "storage", "thermal", "profiles", "diagnostics"].includes(page.topicId)
        sourceComponent: Component { KernelManagerView { mode: page.topicId; appWindow: page.appWindow; currentPage: page.appWindow.pageStack.currentItem === page && settings.topic.id === page.topicId } }
    }
    PagePanel {
        active: Object.keys(settings.bootReport).length > 0
        sourceComponent: Component { UpdateStatusView { report: settings.bootReport } }
    }
    PagePanel {
        active: page.topicId === "kernel" && Object.keys(settings.bootReport).length > 0
        sourceComponent: Component { BootSelection { settings: page.settings } }
    }
    PagePanel {
        active: page.topicId === "updates"
        sourceComponent: Component { SystemUpdatePanel { settings: page.settings; appWindow: page.appWindow } }
    }
    PagePanel {
        active: page.topicId === "changelog"
        sourceComponent: Component { ChangelogView { settings: page.settings } }
    }
    PagePanel {
        active: page.topicId === "usb"
        sourceComponent: Component {
            TileGrid {
                minimumTileWidth: Kirigami.Units.gridUnit * 25
                UsbRoleView { topicPage: page }
                DisplayLinkPanel { settings: page.settings; appWindow: page.appWindow }
            }
        }
    }
    PagePanel {
        active: page.topicId === "nfc"
        sourceComponent: Component { NfcView {} }
    }
    PagePanel {
        active: page.topicId === "infrared"
        sourceComponent: Component { InfraredPanel { settings: page.settings } }
    }
    PagePanel {
        active: page.topicId === "led"
        sourceComponent: Component { NotificationLedPanel { settings: page.settings } }
    }
    PagePanel {
        active: page.topicId === "display"
        sourceComponent: Component { DisplayPreferencesPanel { settings: page.settings; appWindow: page.appWindow } }
    }
    PagePanel {
        active: page.topicId === "about"
        sourceComponent: Component {
            ColumnLayout {
                spacing: Kirigami.Units.largeSpacing

                Controls.BusyIndicator {
                    Layout.alignment: Qt.AlignHCenter
                    running: page.settings.deviceInformation.length === 0
                    visible: running
                }
                Repeater {
                    model: page.settings.deviceInformation
                    delegate: ColumnLayout {
                        id: inventoryGroup
                        required property var modelData
                        Layout.fillWidth: true
                        spacing: Kirigami.Units.mediumSpacing

                        Kirigami.Heading {
                            Layout.fillWidth: true
                            level: 3
                            text: inventoryGroup.modelData.title
                            wrapMode: Text.Wrap
                        }
                        InformationGrid {
                            Layout.maximumWidth: Number.POSITIVE_INFINITY
                            Layout.margins: 0
                            minimumTileWidth: Kirigami.Units.gridUnit * 14
                            loadAsynchronously: !backend.simulation
                            sections: inventoryGroup.modelData.cards
                        }
                    }
                }
            }
        }
    }
    PagePanel {
        active: page.topicId === "about-uke"
        sourceComponent: Component { AboutUke {} }
    }
    PagePanel {
        active: page.topicId === "flashlight"
        sourceComponent: Component { FlashlightPanel { settings: page.settings } }
    }
    PagePanel {
        active: page.isCharging
        sourceComponent: Component { ChargingPanel {
                settings: page.settings
                information: page.information
                currentPage: page.appWindow.pageStack.currentItem === page && settings.topic.id === page.topicId
            } }
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
        active: !page.isCharging && !page.showConnectionsFirst &&
                !["flashlight", "updates", "about", "display", "led"].includes(page.topicId) &&
                page.information.length > 0
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
    FormCard.FormCard {
        visible: page.topicId !== "about-uke"
        maximumWidth: page.readingColumnWidth
        Repeater {
            model: page.isCharging ? page.topicActions : []
            delegate: FormCard.FormButtonDelegate {
                required property var modelData
                text: modelData.id === "charge.mode" ? i18n("Other charging policies")
                    : modelData.id === "charge.limit" ? i18n("Stop threshold only") : modelData.title
                enabled: !backend.readOnly && (modelData.id.startsWith("wake.") || settings.topic.canApply !== false)
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
