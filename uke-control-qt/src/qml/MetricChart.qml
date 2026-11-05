// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard
import org.uke.settings 1.0
FormCard.FormCard {
    id: card
    required property var manager
    required property string metric
    required property string heading
    property string unit: ""
    property int seconds: 120
    property real maximum: 0
    Layout.fillWidth: true
    SafeText { text: card.heading; description: card.unit }
    TimePlot {
        id: plot
        objectName: "metricPlot_" + card.metric
        Layout.fillWidth: true
        Layout.preferredHeight: Kirigami.Units.gridUnit * 10
        Layout.leftMargin: Kirigami.Units.smallSpacing
        Layout.rightMargin: Kirigami.Units.smallSpacing
        foreground: Kirigami.Theme.textColor
        accent: Kirigami.Theme.highlightColor
        report: { let revision = card.manager.revision; return card.manager.plot(card.metric, card.seconds, card.maximum) }
        Accessible.role: Accessible.Chart
        Accessible.name: card.heading + " " + card.unit
        Accessible.description: i18n("Observed values over time; missing samples break the line.")
    }
    Controls.Label {
        Layout.fillWidth: true
        Layout.margins: Kirigami.Units.largeSpacing
        text: plot.cursor.length ? plot.cursor + " " + card.unit : plot.report.points < 2 ? i18n("Waiting for enough samples…") : i18n("Point or tap the graph to inspect a sample.")
        wrapMode: Text.WordWrap
        textFormat: Text.PlainText
    }
}
