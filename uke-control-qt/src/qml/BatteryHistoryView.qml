// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard
import org.uke.settings 1.0
ColumnLayout {
    id: view
    readonly property var report: backend.settings.history
    Layout.fillWidth: true
    spacing: 0
    FormCard.FormHeader { title: i18n("Battery history") }
    FormCard.FormCard {
        Layout.fillWidth: true
        FormCard.FormComboBoxDelegate {
            id: range
            text: i18n("Time range")
            model: [{title:i18n("Last hour"),value:3600},{title:i18n("Last 24 hours"),value:86400}]
            textRole: "title"
            valueRole: "value"
            currentIndex: 1
            Accessible.name: text
        }
        FormCard.FormButtonDelegate {
            objectName: "loadBatteryHistory"
            text: i18n("Read existing history")
            icon.name: themeIcons.names["view-refresh"]
            Accessible.name: text
            enabled: !view.report.busy
            onClicked: backend.settings.loadHistory(range.currentValue)
        }
        Controls.BusyIndicator { Layout.alignment: Qt.AlignHCenter; running: view.report.busy || false; visible: running }
        SafeText {
            text: view.report.message || i18n("History is read from the existing power service. No additional recording service is started.")
            description: view.report.available ? i18n("%1 valid points · %2 gaps", view.report.points, view.report.gaps) : ""
        }
        HistoryPlot {
            objectName: "batteryHistoryPlot"
            Layout.fillWidth: true
            Layout.preferredHeight: Kirigami.Units.gridUnit * 13
            Layout.leftMargin: Kirigami.Units.largeSpacing
            Layout.rightMargin: Kirigami.Units.largeSpacing
            visible: view.report.available || false
            report: view.report
            Accessible.role: Accessible.Graphic
            Accessible.name: i18n("Recorded battery percentage over time")
            Accessible.description: view.report.message || ""
        }
        Repeater {
            model: view.report.segments || []
            delegate: SafeText {
                required property var modelData
                visible: !view.report.available
                text: i18n("Recorded observations")
                description: modelData.length.toString()
            }
        }
    }
}
