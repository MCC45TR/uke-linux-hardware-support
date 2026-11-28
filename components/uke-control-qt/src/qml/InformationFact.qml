// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

GridLayout {
    id: row
    required property var fact

    Layout.fillWidth: true
    Layout.minimumWidth: 0
    columns: width >= Kirigami.Units.gridUnit * 15 && fact.value.length <= 36 ? 2 : 1
    uniformCellWidths: true
    columnSpacing: Kirigami.Units.mediumSpacing
    rowSpacing: 0

    Controls.Label {
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignTop
        text: row.fact.title
        textFormat: Text.PlainText
        wrapMode: Text.Wrap
        Accessible.ignored: true
    }
    Controls.Label {
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignTop
        text: row.fact.value
        textFormat: Text.PlainText
        wrapMode: Text.Wrap
        font.weight: text.length <= 36 ? Font.DemiBold : Font.Normal
        horizontalAlignment: row.columns === 2 ? Text.AlignRight : Text.AlignLeft
        Accessible.name: i18nc("measurement label and value for screen readers", "%1, %2", row.fact.title, text)
    }
}
