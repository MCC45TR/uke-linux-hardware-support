// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

FormCard.FormCardPage {
    id: page
    objectName: "guidePage"
    required property var appWindow
    readonly property bool hasOptions: (backend.guide.parameters || []).length > 0
    readonly property bool hasInspection: Boolean(backend.guide.inspect && !backend.guide.read)
    readonly property int reviewStep: 1 + (hasInspection ? 1 : 0) + (hasOptions ? 1 : 0)
    readonly property string submitText: {
        if (backend.guide.read) return backend.guide.capture ? "Read information" : "Review terminal access…"
        if (backend.guide.diagnostic) return "Review diagnostic…"
        if (backend.guide.id === "channel.set") return "Review channel change…"
        if (backend.guide.id === "update.apply") return "Review installation…"
        switch (backend.guide.effect) {
        case "packages": return "Review software operation…"
        case "maintenance": return "Review maintenance…"
        case "publish": return "Review publication…"
        case "local_file": return "Review file request…"
        default: return "Review preference…"
        }
    }
    readonly property string submitIcon: backend.guide.read && backend.guide.capture ? "view-refresh" : "document-preview"
    title: backend.guide.title || "Device settings"
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
        text: backend.simulation ? "Simulation — sample data only. Device commands and changes are disabled." : "Read-only session. Changes are disabled."
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
        text: (backend.guide.accessLabel || "") + ". " + (backend.guide.read ? "This observation opens in a terminal after you review access." : "The final step opens a terminal for authentication and the operation's result.")
    }
    FormCard.FormHeader { visible: page.hasInspection; title: "1. Check current state" }
    FormCard.FormCard {
        visible: page.hasInspection
        FormCard.FormButtonDelegate {
            objectName: "inspectCurrent"
            text: backend.guide.inspectTerminal ? "Inspect in terminal…" : "Read current state"
            description: backend.guide.inspectTerminal ? "Review access to the current state before choosing a preference." : "Read the reported state to compare with your requested values."
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
        title: backend.guide.read ? "Read options" : (page.hasInspection ? "2. " : "1. ") + "Choose your preference"
    }
    FormCard.FormCard {
        visible: page.hasOptions
        SafeText {
            text: backend.guide.read ? "Choose what to inspect" : "Requested values"
            description: backend.guide.read ? "The result will identify the options used for the read." : "These are draft values. Use the current-state check to see the device's reported settings."
        }
        RequestFields { Layout.fillWidth: true; pageWidth: page.width }
    }
    FormCard.FormHeader {
        visible: !backend.guide.read
        title: page.reviewStep + ". Preview & review"
    }
    FormCard.FormCard {
        visible: backend.guide.preview || false
        FormCard.FormButtonDelegate {
            objectName: "previewRequest"
            text: backend.guide.previewTerminal ? "Preview in terminal…" : "Preview requested values"
            description: backend.guide.previewTerminal ? "Review access, then inspect the software plan before applying." : "Check the requested values before applying a change."
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
            description: backend.guide.read && backend.guide.capture ? "Show the reported information below." : "Review the request, values and access requirements before continuing."
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
            text: "Technical details & requirements"
            icon.name: themeIcons.names["dialog-information"]
            Accessible.name: text
            Accessible.description: details.visible ? "Details expanded" : "Details collapsed"
            onClicked: details.visible = !details.visible
        }
        SafeText {
            id: details
            visible: false
            text: backend.guide.id || ""
            description: (backend.guide.details || "") + "\n\nEffect: " + (backend.guide.role || "") + "\nAccess: " + (backend.guide.accessLabel || "")
        }
    }
}
