// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard
ColumnLayout {
    id: view
    required property var manager
    required property string mode
    required property int seconds
    property var drafts: ({})
    property int firstChart: 0
    property int secondChart: 1
    property bool advanced: false
    readonly property var charts: manager.state.charts || []
    function draft(id, value) { const copy = Object.assign({}, drafts); copy[id] = value; drafts = copy }
    function help(id) {
        const names = {"memory.swappiness":i18n("Controls the preference for reclaiming anonymous memory into swap. This is not a RAM capacity limit."),"memory.ksm":i18n("Scans eligible memory for identical pages. It can save memory while adding CPU work."),"memory.thp":i18n("Choose whether large transparent pages are used always, on application advice, or never."),"memory.mglru":i18n("0 disables MGLRU, 1 enables the basic mechanism, and 7 enables all supported aging features. Availability depends on the running kernel.")}
        return names[id] || ""
    }
    function status(value) {
        const names = {"available":i18n("Read from the kernel"),"not-exported":i18n("This kernel does not export this interface"),"kernel-support-missing":i18n("Requires additional kernel support"),"module":i18n("Kernel module support; check the runtime provider"),"permission-denied":i18n("Access denied"),"no-data":i18n("The driver exports the file but has no data"),"read-failed":i18n("The provider could not be read"),"empty":i18n("The provider returned no value"),"ambiguous-or-not-exported":i18n("No unambiguous backing queue was found")}
        return names[value] || value
    }
    Layout.fillWidth: true
    spacing: 0
    FormCard.FormCard {
        Layout.fillWidth: true
        visible: !!(view.manager.state.powerProfile || "")
        SafeText { text: i18n("System power profile"); description: view.manager.state.powerProfile || "" }
        SafeText { text: i18n("Who controls frequency?"); description: i18n("The desktop power profile, frequency governor and thermal policy can all constrain frequency. A requested limit does not prove that a particular hardware clock was reached.") }
    }
    FormCard.FormHeader { visible: view.charts.length > 0; title: i18n("Live measurements") }
    FormCard.FormCard {
        visible: view.charts.length > 0
        Layout.fillWidth: true
        FormCard.FormComboBoxDelegate { text: i18n("First graph"); model: view.charts; textRole: "title"; currentIndex: Math.min(view.firstChart, view.charts.length - 1); onActivated: index => view.firstChart = index }
        FormCard.FormComboBoxDelegate { visible: view.charts.length > 1; text: i18n("Second graph"); model: view.charts; textRole: "title"; currentIndex: Math.min(view.secondChart, view.charts.length - 1); onActivated: index => view.secondChart = index }
    }
    GridLayout {
        Layout.fillWidth: true
        columns: width > Kirigami.Units.gridUnit * 45 ? 2 : 1
        visible: view.charts.length > 0
        Repeater {
            model: view.charts.length ? [view.charts[Math.min(view.firstChart, view.charts.length - 1)], view.charts[Math.min(view.secondChart, view.charts.length - 1)]] : []
            delegate: ColumnLayout {
                required property var modelData
                required property int index
                visible: index === 0 || view.charts.length > 1
                Layout.fillWidth: true
                SafeText { text: modelData.title; description: view.manager.format((view.manager.state.metrics || {})[modelData.id], modelData.unit) }
                MetricChart { manager: view.manager; metric: modelData.id; heading: modelData.title; unit: modelData.unit; maximum: modelData.maximum; seconds: view.seconds }
            }
        }
    }
    Repeater {
        model: view.manager.state.sections || []
        delegate: ColumnLayout {
            id: section
            required property var modelData
            property bool expanded: false
            Layout.fillWidth: true
            spacing: 0
            FormCard.FormHeader { title: section.modelData.title }
            FormCard.FormCard {
                Layout.fillWidth: true
                Repeater {
                    model: section.modelData.rows
                    delegate: SafeText {
                        required property var modelData
                        required property int index
                        visible: index < 3 || section.expanded
                        text: modelData.title
                        description: (modelData.value || i18n("Not reported")) + (modelData.status === "available" ? "" : "\n" + view.status(modelData.status))
                    }
                }
                FormCard.FormButtonDelegate {
                    visible: section.modelData.rows.length > 3
                    text: section.expanded ? i18n("Show less") : i18n("Show all %1 readings", section.modelData.rows.length)
                    icon.name: themeIcons.names["view-list-details"]
                    onClicked: section.expanded = !section.expanded
                }
            }
        }
    }
    FormCard.FormHeader { visible: (view.manager.state.controls || []).length > 0; title: i18n("Runtime controls") }
    FormCard.FormCard {
        Layout.fillWidth: true
        visible: (view.manager.state.controls || []).length > 0
        FormCard.FormSwitchDelegate { text: i18n("Advanced kernel controls"); description: i18n("Show governor tuning, memory experiments and queue details, including unavailable interfaces."); checked: view.advanced; onToggled: view.advanced = checked }
    }
    Repeater {
        model: view.manager.state.controls || []
        delegate: FormCard.FormCard {
            id: control
            required property var modelData
            readonly property string requested: view.drafts[modelData.id] === undefined ? modelData.value : view.drafts[modelData.id]
            readonly property bool available: modelData.status === "available" && modelData.writable
            visible: view.advanced || (control.available && ["memory.swappiness", "storage.scheduler", "storage.read-ahead"].includes(modelData.id))
            Layout.fillWidth: true
            SafeText { text: control.modelData.title; description: i18n("Current: %1 %2", control.modelData.value || i18n("Not reported"), control.modelData.unit) }
            SafeText { visible: view.help(control.modelData.id).length > 0; text: i18n("What this setting does"); description: view.help(control.modelData.id) }
            SafeText { visible: !control.available; text: i18n("Control unavailable"); description: view.status(control.modelData.status) }
            FormCard.FormComboBoxDelegate {
                visible: control.available && control.modelData.choices.length > 0
                text: i18n("Requested value")
                model: control.modelData.choices
                currentIndex: Math.max(0, control.modelData.choices.indexOf(control.requested))
                onActivated: index => view.draft(control.modelData.id, model[index])
            }
            FormCard.FormSwitchDelegate { visible: control.available && control.modelData.id === "memory.ksm"; text: i18n("Merge identical pages"); checked: control.requested === "1"; onToggled: view.draft(control.modelData.id, checked ? "1" : "0") }
            RowLayout {
                visible: control.available && control.modelData.choices.length === 0 && control.modelData.id !== "memory.ksm"
                Layout.fillWidth: true
                Layout.margins: Kirigami.Units.largeSpacing
                Controls.Label { Layout.fillWidth: true; text: i18n("Requested value (%1–%2)", control.modelData.minimum, control.modelData.maximum); wrapMode: Text.WordWrap }
                Controls.SpinBox {
                    objectName: "resourceValue_" + control.modelData.id
                    from: control.modelData.minimum; to: control.modelData.maximum
                    stepSize: control.modelData.id.endsWith("read-ahead") ? 4 : 1
                    value: Number(control.requested)
                    editable: true
                    onValueModified: view.draft(control.modelData.id, String(value))
                    Accessible.name: control.modelData.title
                }
            }
            FormCard.FormButtonDelegate {
                text: i18n("Review change")
                icon.name: themeIcons.names["configure"]
                enabled: control.available && control.requested !== control.modelData.value && !view.manager.busy
                onClicked: view.manager.prepareResource(control.modelData.id, control.requested)
            }
        }
    }
    FormCard.FormHeader { title: i18n("Session events") }
    FormCard.FormCard {
        Layout.fillWidth: true
        SafeText { visible: !(view.manager.state.events || []).length; text: i18n("No changes recorded"); description: i18n("Profile, control and pressure changes are recorded while these pages are being observed. The list holds at most 100 events and is not a background recorder.") }
        Repeater { model: (view.manager.state.events || []).slice(0, 12); delegate: SafeText { required property var modelData; text: modelData.time + " · " + modelData.title; description: modelData.value } }
    }
}
