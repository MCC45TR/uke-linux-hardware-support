// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard
ColumnLayout {
    id: editor
    required property real pageWidth
    Layout.fillWidth: true
    spacing: 0
    Kirigami.InlineMessage {
        Layout.fillWidth: true
        visible: backend.message.length > 0
        type: backend.messageError ? Kirigami.MessageType.Warning : Kirigami.MessageType.Information
        text: backend.message
    }
    FormCard.FormHeader { title: backend.guide.title || "" }
    FormCard.FormCard {
        visible: (backend.guide.parameters || []).length > 0
        SafeText { text: i18n("Requested preference"); description: i18n("These values are a request. They do not represent a verified current setting.") }
        RequestFields { Layout.fillWidth: true; pageWidth: editor.pageWidth }
        FormCard.FormButtonDelegate {
            visible: backend.guide.id === "boot.set-slot" && ["a", "b"].includes(backend.settings.bootReport.current_slot)
            text: i18n("Select the other slot")
            Accessible.name: text
            onClicked: backend.setDraft("slot", backend.settings.bootReport.current_slot === "a" ? "b" : "a")
        }
    }
    FormCard.FormCard {
        visible: backend.guide.preview || false
        FormCard.FormButtonDelegate {
            text: i18n("Preview")
            icon.name: themeIcons.names["document-preview"]
            Accessible.name: text
            onClicked: backend.readRelated("preview")
        }
    }
    FormCard.FormCard {
        FormCard.FormButtonDelegate {
            objectName: "topicApply"
            text: backend.guide.read ? i18n("Read information") : i18n("Review and apply…")
            description: i18n("The existing authentication and device checks remain in the terminal.")
            icon.name: themeIcons.names["system-run"]
            Accessible.name: text
            enabled: !backend.observation.running && backend.settings.topic.canApply !== false
            onClicked: backend.submit()
        }
    }
    Loader {
        Layout.fillWidth: true
        active: backend.observation.title !== undefined
        sourceComponent: Component { ObservationView {} }
    }
}
