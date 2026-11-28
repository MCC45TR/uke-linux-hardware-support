// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

ColumnLayout {
    id: view
    required property var settings
    readonly property var report: settings.systemErrors
    readonly property var boots: report.boots ?? []
    readonly property var boot: boots[bootSelector.currentIndex]
    readonly property var rows: {
        const term = search.text.trim().toLocaleLowerCase()
        const level = severity.currentIndex
        return (boot?.rows ?? []).filter(row =>
            (level === 0 || row.level === severity.model[level].value)
            && (!term || row.message.toLocaleLowerCase().includes(term)
                || row.source.toLocaleLowerCase().includes(term)))
    }
    property int visibleRows: 30
    spacing: Kirigami.Units.largeSpacing

    SettingsSection {
        heading: i18n("System journal")
        GridLayout {
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.largeSpacing
            columns: width >= Kirigami.Units.gridUnit * 30 ? 3 : 1

            Controls.ComboBox {
                id: bootSelector
                Layout.fillWidth: true
                Accessible.name: i18n("Boot")
                model: view.boots.map(item => i18n("%1 · %2", item.title, item.date))
                onActivated: view.visibleRows = 30
            }
            Controls.ComboBox {
                id: severity
                Layout.fillWidth: true
                Accessible.name: i18n("Severity")
                textRole: "label"
                model: [
                    { label: i18n("All findings"), value: "" },
                    { label: i18n("Errors"), value: "error" },
                    { label: i18n("Warnings"), value: "warning" },
                    { label: i18n("Error text in other messages"), value: "message" }
                ]
                onActivated: view.visibleRows = 30
            }
            Kirigami.SearchField {
                id: search
                Layout.fillWidth: true
                placeholderText: i18n("Search messages or services")
                onTextChanged: view.visibleRows = 30
            }
        }
        Controls.BusyIndicator {
            Layout.alignment: Qt.AlignHCenter
            visible: running
            running: view.report.busy === true
        }
        SettingsNote {
            visible: (view.report.message ?? "").length > 0
            text: view.report.message ?? ""
        }
        SettingsNote {
            visible: view.boot?.limited === true
            text: i18n("Search currently covers the loaded findings. You can read older records below.")
        }
        SettingsNote {
            visible: !view.report.busy && view.boots.length > 0 && view.boots.length < 3
            text: i18np("One boot is available in the journal.", "%1 boots are available in the journal.", view.boots.length)
        }
        Controls.Button {
            Layout.alignment: Qt.AlignRight
            Layout.margins: Kirigami.Units.largeSpacing
            text: i18n("Scan again")
            icon.name: "view-refresh"
            enabled: !view.report.busy
            onClicked: {
                view.visibleRows = 30
                view.settings.scanSystemErrors()
            }
        }
    }

    SettingsSection {
        heading: i18np("One finding", "%1 findings", view.rows.length)
        visible: view.boot !== undefined
        SettingsNote {
            visible: view.rows.length === 0
            text: i18n("No matching errors or warnings.")
        }
        Repeater {
            model: view.rows.slice(0, view.visibleRows)
            delegate: ColumnLayout {
                id: finding
                required property var modelData
                Layout.fillWidth: true
                Layout.margins: Kirigami.Units.largeSpacing
                Controls.Label {
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    textFormat: Text.PlainText
                    font.bold: true
                    text: finding.modelData.source.length > 0
                          ? i18n("%1 · %2", finding.modelData.source, finding.modelData.time)
                          : finding.modelData.time
                }
                Controls.Label {
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    textFormat: Text.PlainText
                    text: finding.modelData.message
                }
                Kirigami.Separator { Layout.fillWidth: true }
            }
        }
        Controls.Button {
            Layout.alignment: Qt.AlignHCenter
            Layout.margins: Kirigami.Units.largeSpacing
            visible: view.visibleRows < view.rows.length
            text: i18n("Show more")
            onClicked: view.visibleRows += 30
        }
        Controls.Button {
            Layout.alignment: Qt.AlignHCenter
            Layout.margins: Kirigami.Units.largeSpacing
            visible: view.boot?.limited === true
            enabled: !view.report.busy
            text: i18n("Read older findings")
            onClicked: view.settings.scanSystemErrors(bootSelector.currentIndex)
        }
    }
}
