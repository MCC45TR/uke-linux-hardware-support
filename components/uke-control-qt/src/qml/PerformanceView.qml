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
    readonly property var targets: manager.devices.filter(d => d.kind === mode)
    property int selected: 0
    property bool details: false
    readonly property var device: targets.length > selected ? targets[selected] : ({})
    readonly property var reading: manager.state.values ? (manager.state.values[device.id] || {}) : ({})
    Layout.fillWidth: true
    spacing: Kirigami.Units.largeSpacing
    SettingsSection {
        heading: i18n("Current performance")
        FormCard.FormComboBoxDelegate {
            objectName: "frequencyDevice"
            visible: view.targets.length > 1
            text: i18n("CPU cluster")
            model: view.targets.map(d => ({title: i18n("CPUs %1", d.cpus), value: d.id}))
            textRole: "title"
            currentIndex: view.selected
            onActivated: index => view.selected = index
        }
        SafeText { text: i18n("Driver-reported frequency"); description: view.manager.format(view.reading.frequency, "MHz") }
        SafeText { visible: view.mode === "cpu"; text: i18n("Cluster load"); description: view.manager.format(view.reading.load, "%") }
        SafeText {
            text: view.mode === "cpu" ? i18n("Highest reported CPU temperature") : i18n("Highest reported GPU temperature")
            description: view.manager.format(view.manager.state[view.mode === "cpu" ? "cpuTemperature" : "gpuTemperature"], "°C")
        }
        SafeText { visible: view.details; text: i18n("Frequency driver"); description: view.device.driver || i18n("Not reported") }
        SafeText {
            visible: view.details
            text: i18n("Measurement meaning")
            description: view.mode === "cpu" ? i18n("CPU frequency is reported by the scaling driver and can differ from the hardware clock. Load is computed from successive kernel CPU counters.") : i18n("GPU frequency is reported by devfreq. Clock frequency is not utilization. Available application engine counters are shown separately below.")
        }
        FormCard.FormButtonDelegate { text: view.details ? i18n("Less information") : i18n("More information"); icon.name: themeIcons.names["view-list-details"]; onClicked: view.details = !view.details }
    }
    TileGrid {
        maximumColumns: 2
        minimumTileWidth: Kirigami.Units.gridUnit * 24
        MetricChart { manager: view.manager; metric: (view.device.id || "") + "/frequency"; heading: i18n("Frequency history"); unit: "MHz"; seconds: view.seconds; maximum: view.device.frequencies && view.device.frequencies.length ? Math.max(...view.device.frequencies.map(Number)) / (view.mode === "cpu" ? 1000 : 1000000) : 0 }
        MetricChart { manager: view.manager; metric: view.mode === "cpu" ? (view.device.id || "") + "/load" : "gpuTemperature"; heading: view.mode === "cpu" ? i18n("CPU load history") : i18n("GPU temperature history"); unit: view.mode === "cpu" ? "%" : "°C"; seconds: view.seconds; maximum: view.mode === "cpu" ? 100 : 0 }
    }
    SettingsSection {
        visible: view.mode === "cpu" && (view.manager.state.cores || []).length > 0
        heading: i18n("CPU cores")
        TileColumns {
            Layout.margins: Kirigami.Units.largeSpacing
            minimumTileWidth: Kirigami.Units.gridUnit * 13
            items: view.mode === "cpu" ? view.manager.state.cores || [] : []
            tile: ColumnLayout {
                readonly property var core: tileData
                Controls.Label { text: core.id; font.weight: Font.DemiBold }
                Controls.ProgressBar {
                    Layout.fillWidth: true
                    visible: core.load !== null && core.load !== undefined
                    value: visible ? core.load / 100 : 0
                    Accessible.name: core.id + " " + view.manager.format(core.load, "%")
                }
                Controls.Label { text: view.manager.format(core.load, "%") }
            }
        }
    }
    FrequencyControls { Layout.fillWidth: true; visible: view.targets.length > 0; manager: view.manager; deviceId: view.device.id || "" }
    SafeText { visible: view.targets.length === 0 && !view.manager.busy; text: i18n("No matching frequency provider"); description: i18n("This kernel does not export a supported frequency control for this device.") }
}
