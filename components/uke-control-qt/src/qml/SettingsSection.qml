// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

ColumnLayout {
    id: section
    property string heading: ""
    default property alias rows: card.delegates
    Layout.fillWidth: true
    Layout.minimumWidth: 0
    Layout.alignment: Qt.AlignTop
    spacing: Kirigami.Units.smallSpacing

    children: [
        Kirigami.Heading {
            visible: text.length > 0
            Layout.fillWidth: true
            text: section.heading
            level: 3
            wrapMode: Text.Wrap
        },
        FormCard.FormCard {
            id: card
            Layout.minimumWidth: 0
            maximumWidth: Math.max(0, width - 1)
        }
    ]
}
