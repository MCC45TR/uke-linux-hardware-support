// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

ColumnLayout {
    id: fields
    required property real pageWidth
    spacing: 0
    Repeater {
        model: backend.guide.parameters || []
        delegate: ColumnLayout {
            id: fieldRow
            required property var modelData
            Layout.fillWidth: true
            spacing: 0
            Loader {
                Layout.fillWidth: true
                sourceComponent: fieldRow.modelData.choices.length > 0 ? choiceField : fieldRow.modelData.integer ? integerField : textField
                Component {
                    id: integerField
                    FormCard.FormSpinBoxDelegate {
                        objectName: "parameter_" + fieldRow.modelData.key
                        label: fieldRow.modelData.title
                        Accessible.name: label
                        Accessible.description: fieldRow.modelData.description
                        from: fieldRow.modelData.minimum
                        to: fieldRow.modelData.maximum
                        value: Number(fieldRow.modelData.value)
                        onValueModified: backend.setDraft(fieldRow.modelData.key, String(value))
                    }
                }
                Component {
                    id: choiceField
                    FormCard.FormComboBoxDelegate {
                        objectName: "parameter_" + fieldRow.modelData.key
                        text: fieldRow.modelData.title
                        Accessible.name: text
                        description: currentIndex >= 0 && model[currentIndex] ? model[currentIndex].description : fieldRow.modelData.description
                        model: fieldRow.modelData.choices
                        textRole: "title"
                        valueRole: "value"
                        displayMode: fields.pageWidth < Kirigami.Units.gridUnit * 35 ? FormCard.FormComboBoxDelegate.Dialog : FormCard.FormComboBoxDelegate.ComboBox
                        currentIndex: {
                            for (let i = 0; i < model.length; ++i)
                                if (model[i].value === fieldRow.modelData.value) return i
                            return 0
                        }
                        onActivated: index => backend.setDraft(fieldRow.modelData.key, model[index].value)
                    }
                }
                Component {
                    id: textField
                    FormCard.FormTextFieldDelegate {
                        objectName: "parameter_" + fieldRow.modelData.key
                        label: fieldRow.modelData.title
                        Accessible.name: label
                        Accessible.description: fieldRow.modelData.description
                        text: fieldRow.modelData.value
                        maximumLength: 4096
                        onTextEdited: backend.setDraft(fieldRow.modelData.key, text)
                    }
                }
            }
            FormCard.FormSectionText {
                visible: fieldRow.modelData.choices.length === 0
                text: fieldRow.modelData.description + (fieldRow.modelData.integer ? " Allowed range: " + fieldRow.modelData.minimum + "–" + fieldRow.modelData.maximum + "." : "")
                textFormat: Text.PlainText
            }
        }
    }
    FormCard.FormButtonDelegate {
        objectName: "resetDraft"
        text: backend.guide.read ? "Reset read options" : "Reset draft values"
        description: "Use the suggested starting values."
        icon.name: themeIcons.names["edit-clear"]
        Accessible.name: text
        onClicked: backend.resetDraft()
    }
}
