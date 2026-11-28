// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

ColumnLayout {
    id: editor

    readonly property bool canSubmit: !backend.observation.running && backend.settings.topic.canApply !== false
        && (!backend.readOnly || backend.guide.read)

    Layout.fillWidth: true
    spacing: 0

    Kirigami.InlineMessage {
        Layout.fillWidth: true
        visible: backend.message.length > 0
        type: backend.messageError ? Kirigami.MessageType.Error : Kirigami.MessageType.Information
        text: backend.message
    }

    FormCard.FormHeader { title: backend.guide.title || "" }
    FormCard.FormCard {
        FormCard.FormSectionText {
            visible: text.length > 0
            text: backend.guide.description || ""
            textFormat: Text.PlainText
            Layout.bottomMargin: 0
        }
        RequestFields { Layout.fillWidth: true }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.largeSpacing
            spacing: Kirigami.Units.mediumSpacing

            Controls.Label {
                Layout.fillWidth: true
                visible: !backend.guide.read
                text: i18n("Review your choices before applying them in the terminal.")
                wrapMode: Text.Wrap
            }

            GridLayout {
                Layout.fillWidth: true
                columns: preview.visible && width >= preview.implicitWidth + applyButton.implicitWidth + columnSpacing ? 2 : 1
                columnSpacing: Kirigami.Units.mediumSpacing
                rowSpacing: Kirigami.Units.smallSpacing

                Controls.Button {
                    id: preview
                    Layout.alignment: Qt.AlignLeft
                    visible: backend.guide.preview || false
                    text: i18n("Preview")
                    icon.name: themeIcons.names["document-preview"]
                    enabled: editor.canSubmit
                    onClicked: backend.readRelated("preview")
                }
                Controls.Button {
                    id: applyButton
                    objectName: "topicApply"
                    Layout.alignment: Qt.AlignLeft
                    text: backend.guide.read ? i18n("Read information") : i18n("Review and apply…")
                    icon.name: themeIcons.names[backend.guide.read ? "view-refresh" : "document-preview"]
                    enabled: editor.canSubmit
                    onClicked: backend.submit()
                }
            }
        }
    }

    Loader {
        Layout.fillWidth: true
        active: backend.observation.title !== undefined
        sourceComponent: Component { ObservationView {} }
    }
}
