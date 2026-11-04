// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Dialogs
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
            readonly property string selectedValue: backend.draftValues[modelData.key] || ""
            Layout.fillWidth: true
            spacing: 0
            Loader {
                Layout.fillWidth: true
                sourceComponent: fieldRow.modelData.choices.length > 0
                    ? (fieldRow.modelData.choices.length <= 3 ? radioField : choiceField)
                    : fieldRow.modelData.kind === "color" ? colorField
                    : fieldRow.modelData.kind === "path" ? fileField
                    : fieldRow.modelData.kind === "directory" ? folderField
                    : fieldRow.modelData.integer ? integerField : textField
                Component {
                    id: radioField
                    ColumnLayout {
                        objectName: "parameter_" + fieldRow.modelData.key
                        Layout.fillWidth: true
                        spacing: 0
                        Accessible.name: fieldRow.modelData.title
                        SafeText { text: fieldRow.modelData.title }
                        Controls.ButtonGroup { id: options }
                        Repeater {
                            model: fieldRow.modelData.choices
                            delegate: FormCard.FormRadioDelegate {
                                required property var modelData
                                Layout.fillWidth: true
                                text: modelData.title
                                description: modelData.description || ""
                                Accessible.name: text
                                Controls.ButtonGroup.group: options
                                checked: fieldRow.selectedValue === modelData.value
                                onClicked: backend.setDraft(fieldRow.modelData.key, modelData.value)
                            }
                        }
                    }
                }
                Component {
                    id: integerField
                    FormCard.FormSpinBoxDelegate {
                        objectName: "parameter_" + fieldRow.modelData.key
                        label: fieldRow.modelData.title
                        Accessible.name: label
                        from: fieldRow.modelData.minimum
                        to: fieldRow.modelData.maximum
                        value: Number(fieldRow.selectedValue)
                        onValueModified: backend.setDraft(fieldRow.modelData.key, String(value))
                    }
                }
                Component {
                    id: choiceField
                    FormCard.FormComboBoxDelegate {
                        objectName: "parameter_" + fieldRow.modelData.key
                        text: fieldRow.modelData.title
                        Accessible.name: text
                        model: fieldRow.modelData.choices
                        textRole: "title"
                        valueRole: "value"
                        description: currentIndex >= 0 ? (model[currentIndex].description || "") : ""
                        displayMode: fields.pageWidth < Kirigami.Units.gridUnit * 35 ? FormCard.FormComboBoxDelegate.Dialog : FormCard.FormComboBoxDelegate.ComboBox
                        currentIndex: {
                            for (let i = 0; i < model.length; ++i) if (model[i].value === fieldRow.selectedValue) return i
                            return -1
                        }
                        onActivated: index => backend.setDraft(fieldRow.modelData.key, model[index].value)
                    }
                }
                Component {
                    id: colorField
                    FormCard.FormColorDelegate {
                        objectName: "parameter_" + fieldRow.modelData.key
                        text: fieldRow.modelData.title
                        color: fieldRow.selectedValue
                        Accessible.name: text
                        onColorChanged: if (String(color).length === 7) backend.setDraft(fieldRow.modelData.key, String(color))
                    }
                }
                Component {
                    id: fileField
                    FormCard.FormFileDelegate {
                        objectName: "parameter_" + fieldRow.modelData.key
                        label: fieldRow.modelData.title
                        Accessible.name: label
                        fileMode: ["before", "after", "snapshot", "review"].includes(fieldRow.modelData.key) ? FileDialog.OpenFile : FileDialog.SaveFile
                        selectedFile: fieldRow.selectedValue.startsWith("/") ? "file://" + fieldRow.selectedValue : ""
                        onSelectedFileChanged: if (selectedFile.toString().length) backend.setDraft(fieldRow.modelData.key, backend.settings.localPath(selectedFile))
                    }
                }
                Component {
                    id: folderField
                    FormCard.FormFolderDelegate {
                        objectName: "parameter_" + fieldRow.modelData.key
                        label: fieldRow.modelData.title
                        Accessible.name: label
                        onSelectedFolderChanged: if (selectedFolder.toString().length) backend.setDraft(fieldRow.modelData.key, backend.settings.localPath(selectedFolder))
                    }
                }
                Component {
                    id: textField
                    FormCard.FormTextFieldDelegate {
                        objectName: "parameter_" + fieldRow.modelData.key
                        label: fieldRow.modelData.title
                        Accessible.name: label
                        text: fieldRow.selectedValue
                        maximumLength: 4096
                        onTextEdited: backend.setDraft(fieldRow.modelData.key, text)
                    }
                }
            }
        }
    }
}
