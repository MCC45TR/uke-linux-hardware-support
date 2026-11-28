// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard
import org.uke.settings 1.0
SettingsSection {
    id: view
    readonly property var report: backend.settings.history
    Layout.fillWidth: true
    heading: i18n("Battery history")
    Component.onCompleted: backend.settings.loadHistory(86400)
        FormCard.FormComboBoxDelegate {
            id: range
            text: i18n("Time range")
            model: [{title:i18n("Last hour"),value:3600},{title:i18n("Last 24 hours"),value:86400}]
            textRole: "title"
            valueRole: "value"
            currentIndex: 1
            Accessible.name: text
            onActivated: backend.settings.loadHistory(currentValue)
        }
        FormCard.FormButtonDelegate {
            objectName: "loadBatteryHistory"
            text: i18n("Refresh history")
            icon.name: themeIcons.names["view-refresh"]
            Accessible.name: text
            enabled: !view.report.busy
            onClicked: backend.settings.loadHistory(range.currentValue)
        }
        Controls.BusyIndicator { Layout.alignment: Qt.AlignHCenter; running: view.report.busy || false; visible: running }
        SettingsNote {
            visible: text.length > 0
            text: view.report.message || ""
        }
        HistoryPlot {
            objectName: "batteryHistoryPlot"
            Layout.fillWidth: true
            Layout.preferredHeight: Kirigami.Units.gridUnit * 13
            Layout.leftMargin: Kirigami.Units.largeSpacing
            Layout.rightMargin: Kirigami.Units.largeSpacing
            visible: view.report.available || false
            report: view.report
            foreground: Kirigami.Theme.textColor
            accent: Kirigami.Theme.highlightColor
            Accessible.role: Accessible.Graphic
            Accessible.name: i18n("Recorded battery percentage over time")
            Accessible.description: view.report.message || ""
        }
}
