// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard
ColumnLayout {
    id: editor
    required property var manager
    required property string deviceId
    property string field: "maximum"
    property string requested: ""
    property bool dirty: false
    function reset() { requested = manager.current(deviceId, field); dirty = false }
    Component.onCompleted: reset()
    onDeviceIdChanged: reset()
    Connections { target: editor.manager; function onDevicesChanged() { if (!editor.dirty) editor.reset() } }
    Layout.fillWidth: true
    spacing: 0
    FormCard.FormHeader { title: i18n("Frequency policy") }
    FormCard.FormCard {
        Layout.fillWidth: true
        SafeText { text: i18n("Current limits"); description: {
            let revision = editor.manager.revision
            const unit = editor.deviceId.startsWith("cpu:") ? 1000 : 1000000
            const lo = editor.manager.current(editor.deviceId, "minimum")
            const hi = editor.manager.current(editor.deviceId, "maximum")
            return (lo === "0" ? i18n("Driver default") : editor.manager.format(Number(lo) / unit, "MHz")) + " – " + (hi === "0" ? i18n("Driver default") : editor.manager.format(Number(hi) / unit, "MHz"))
        } }
        SafeText { text: i18n("Current governor"); description: { let revision = editor.manager.revision; return editor.manager.current(editor.deviceId, "governor") } }
        FormCard.FormComboBoxDelegate {
            text: i18n("Setting to change")
            model: [{title: i18n("Maximum frequency"), value: "maximum"}, {title: i18n("Minimum frequency"), value: "minimum"}, {title: i18n("Governor"), value: "governor"}]
            textRole: "title"
            currentIndex: editor.field === "maximum" ? 0 : editor.field === "minimum" ? 1 : 2
            onActivated: index => { editor.field = model[index].value; editor.reset() }
        }
        FormCard.FormComboBoxDelegate {
            objectName: "frequencyValue"
            text: i18n("Requested value")
            model: editor.manager.choices(editor.deviceId, editor.field)
            textRole: "title"
            valueRole: "value"
            currentIndex: { for (let i = 0; i < model.length; ++i) if (model[i].value === editor.requested) return i; return -1 }
            onActivated: index => { editor.requested = model[index].value; editor.dirty = true }
        }
        SafeText {
            text: i18n("Scope of the change")
            description: i18n("Only driver-advertised frequencies and automatic governors are offered. Thermal limits remain in control. Changes are not saved for the next boot.")
        }
        FormCard.FormButtonDelegate {
            objectName: "reviewFrequency"
            text: i18n("Review frequency change")
            icon.name: themeIcons.names["document-preview"]
            enabled: editor.requested.length > 0 && !backend.readOnly && !editor.manager.busy && editor.requested !== editor.manager.current(editor.deviceId, editor.field)
            onClicked: editor.manager.prepare(editor.deviceId, editor.field, editor.requested)
        }
    }
}
