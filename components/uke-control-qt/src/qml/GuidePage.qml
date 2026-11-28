// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

FormCard.FormCardPage {
    id: page
    objectName: "guidePage"
    required property var appWindow
    readonly property bool hasOptions: (backend.guide.parameters || []).length > 0
    readonly property bool hasInspection: Boolean(backend.guide.inspect && !backend.guide.read)
    readonly property string submitText: {
        if (backend.guide.read) return backend.guide.capture ? i18n("Read information") : i18n("Review terminal access…")
        if (backend.guide.id === "boot.set-slot") return i18n("Review slot selection…")
        if (backend.guide.diagnostic) return i18n("Review diagnostic…")
        if (backend.guide.id === "channel.set") return i18n("Review channel change…")
        if (backend.guide.id === "update.apply") return i18n("Review installation…")
        switch (backend.guide.effect) {
        case "packages": return i18n("Review software operation…")
        case "maintenance": return i18n("Review maintenance…")
        case "publish": return i18n("Review publication…")
        case "local_file": return i18n("Review file request…")
        default: return i18n("Review preference…")
        }
    }
    readonly property string submitIcon: backend.guide.read && backend.guide.capture ? "view-refresh" : "document-preview"
    title: backend.guide.title || i18n("Settings")
    actions: [
        Kirigami.Action {
            text: page.submitText
            icon.name: themeIcons.names[page.submitIcon]
            enabled: !backend.observation.running
            onTriggered: backend.submit()
        }
    ]
    Kirigami.InlineMessage {
        objectName: "sessionNotice"
        Layout.fillWidth: true
        visible: backend.simulation || backend.readOnly
        type: backend.simulation ? Kirigami.MessageType.Warning : Kirigami.MessageType.Information
        text: backend.simulation ? i18n("Simulation — sample data only. Changes are disabled.") : i18n("Read-only session")
    }
    Kirigami.InlineMessage {
        Layout.fillWidth: true
        visible: backend.message.length > 0
        type: backend.messageError ? Kirigami.MessageType.Error : Kirigami.MessageType.Information
        text: backend.message
    }
    SafeText {
        icon.name: themeIcons.names[backend.guide.icon || "dialog-information"]
        text: (backend.guide.areaTitle || "") + " · " + (backend.guide.role || "")
        description: backend.guide.description || ""
    }
    FormCard.FormSectionText {
        visible: !backend.guide.capture || backend.guide.administrator || false
        text: backend.guide.read ? i18n("Review access before reading in the terminal.")
                                 : i18n("The terminal handles authentication and shows the result.")
    }
    FormCard.FormHeader { visible: page.hasInspection; title: i18n("Current state") }
    FormCard.FormCard {
        visible: page.hasInspection
        FormCard.FormButtonDelegate {
            objectName: "inspectCurrent"
            text: backend.guide.inspectTerminal ? i18n("Inspect in terminal…") : i18n("Read current state")
            Accessible.name: text
            icon.name: themeIcons.names["view-list-details"]
            enabled: !backend.observation.running
            onClicked: backend.readRelated("inspect")
        }
    }
    Loader {
        Layout.fillWidth: true
        active: backend.observation.title !== undefined && backend.observation.origin === "inspect"
        sourceComponent: observationComponent
    }
    FormCard.FormHeader {
        visible: page.hasOptions
        title: backend.guide.read ? i18n("Read options") : i18n("Requested values")
    }
    FormCard.FormCard {
        visible: page.hasOptions
        RequestFields { Layout.fillWidth: true }
    }
    FormCard.FormHeader {
        visible: !backend.guide.read
        title: i18n("Review")
    }
    FormCard.FormCard {
        visible: backend.guide.preview || false
        FormCard.FormButtonDelegate {
            objectName: "previewRequest"
            text: backend.guide.previewTerminal ? i18n("Preview in terminal…") : i18n("Preview requested values")
            Accessible.name: text
            icon.name: themeIcons.names["document-preview"]
            enabled: !backend.observation.running
            onClicked: backend.readRelated("preview")
        }
    }
    Loader {
        Layout.fillWidth: true
        active: backend.observation.title !== undefined && backend.observation.origin === "preview"
        sourceComponent: observationComponent
    }
    FormCard.FormCard {
        FormCard.FormButtonDelegate {
            objectName: "submitRequest"
            text: page.submitText
            Accessible.name: text
            icon.name: themeIcons.names[page.submitIcon]
            enabled: !backend.observation.running
            onClicked: backend.submit()
        }
    }
    Loader {
        Layout.fillWidth: true
        active: backend.observation.title !== undefined && backend.observation.origin === "read"
        sourceComponent: observationComponent
    }
    Component { id: observationComponent; ObservationView { Layout.fillWidth: true } }
    FormCard.FormCard {
        FormCard.FormButtonDelegate {
            objectName: "toggleAccessDetails"
            text: i18n("Technical details & requirements")
            icon.name: themeIcons.names["dialog-information"]
            Accessible.name: text
            Accessible.description: details.visible ? i18n("Details expanded") : i18n("Details collapsed")
            onClicked: details.visible = !details.visible
        }
        SafeText {
            id: details
            visible: false
            text: backend.guide.id || ""
            description: (backend.guide.details || "") + "\n\n"
                + i18n("Effect: %1", backend.guide.role || "") + "\n"
                + i18n("Access: %1", backend.guide.accessLabel || "")
        }
    }
}
