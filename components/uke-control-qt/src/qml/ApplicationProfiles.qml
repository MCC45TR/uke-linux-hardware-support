// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard
ColumnLayout {
    id: view
    required property var manager
    readonly property var apps: manager.state.applications || []
    property string preset: manager.state.lastPreset || "normal"
    readonly property var presets: [{title:i18n("Normal"),value:"normal"},{title:i18n("Prioritized"),value:"priority"},{title:i18n("Background"),value:"background"},{title:i18n("Resource limited"),value:"limited"}]
    Layout.fillWidth: true
    spacing: Kirigami.Units.largeSpacing
    SettingsSection {
        heading: i18n("Application resource profiles")
        SafeText { text: i18n("Scope"); description: i18n("Choose an active application managed by your desktop session. CPU weight shares processor time during contention. Memory high is a reclaim threshold. A 50% CPU quota is added to the limited preset only when the kernel exports CPU bandwidth control. Changes last for this application invocation; the next invocation needs a new review.") }
        FormCard.FormComboBoxDelegate { text: i18n("Preset"); model: view.presets; textRole: "title"; currentIndex: view.presets.findIndex(p => p.value === view.preset); onActivated: index => view.preset = view.presets[index].value }
        SafeText { visible: !view.apps.length; text: i18n("No managed applications found"); description: i18n("Open an application from the desktop application launcher, then refresh. System services and applications owned by other users are excluded.") }
    }
    TileColumns {
        minimumTileWidth: Kirigami.Units.gridUnit * 21
        items: view.apps
        tile: SettingsSection {
            readonly property var modelData: tileData
            heading: modelData.title
            SafeText { text: i18n("Current profile limits"); description: i18n("CPU weight: %1 · Memory high: %2", modelData.weight, modelData.memoryHigh) }
            SafeText { text: i18n("Current memory use"); description: view.manager.format(((view.manager.state.applicationValues || {})[modelData.unit] || {}).memory, "MiB") }
            SafeText { text: i18n("Kernel controls"); description: i18n("CPU quota: %1 · Utilization clamp: %2 · I/O limit: %3", modelData.quotaSupported ? i18n("Available") : i18n("Requires kernel support"), modelData.uclampSupported ? i18n("Available") : i18n("Requires kernel support"), modelData.ioLimitSupported ? i18n("Available") : i18n("Requires kernel support")) }
            FormCard.FormButtonDelegate { text: i18n("Review %1 preset", view.preset); icon.name: themeIcons.names["configure"]; enabled: !view.manager.busy; onClicked: view.manager.prepareApplication(modelData.unit, view.preset) }
        }
    }
}
