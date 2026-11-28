// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.AbstractCard {
    id: tile

    property bool compactText: false
    property string description: ""
    property string status: ""
    Layout.fillWidth: true
    Layout.fillHeight: true
    Layout.minimumWidth: 0
    Layout.minimumHeight: Kirigami.Units.gridUnit * 4
    focusPolicy: Qt.StrongFocus
    showClickFeedback: true
    highlighted: visualFocus
    Accessible.name: text
    Accessible.description: status.length ? description + "\n" + status : description
    Keys.onReturnPressed: clicked()
    Keys.onEnterPressed: clicked()
    Accessible.onPressAction: clicked()

    contentItem: RowLayout {
        spacing: Kirigami.Units.mediumSpacing

        Kirigami.Icon {
            Layout.alignment: Qt.AlignTop
            Layout.preferredWidth: Kirigami.Units.iconSizes.medium
            Layout.preferredHeight: Layout.preferredWidth
            source: tile.icon.name
            Accessible.ignored: true
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            spacing: Kirigami.Units.smallSpacing

            Kirigami.Heading {
                Layout.fillWidth: true
                level: 4
                font.weight: Font.DemiBold
                text: tile.text
                textFormat: Text.PlainText
                wrapMode: Text.Wrap
                maximumLineCount: tile.compactText ? 2 : 2147483647
                elide: tile.compactText ? Text.ElideRight : Text.ElideNone
                Accessible.ignored: true
            }

            Controls.Label {
                Layout.fillWidth: true
                visible: text.length > 0
                text: tile.description
                textFormat: Text.PlainText
                wrapMode: Text.Wrap
                maximumLineCount: tile.compactText ? 3 : 2147483647
                elide: tile.compactText ? Text.ElideRight : Text.ElideNone
                Accessible.ignored: true
            }

            Controls.Label {
                Layout.fillWidth: true
                visible: text.length > 0
                text: tile.status
                textFormat: Text.PlainText
                wrapMode: Text.Wrap
                font: Kirigami.Theme.smallFont
                Accessible.ignored: true
            }
        }

        Kirigami.Icon {
            Layout.preferredWidth: Kirigami.Units.iconSizes.small
            Layout.preferredHeight: Layout.preferredWidth
            source: tile.mirrored ? "go-previous-symbolic" : "go-next-symbolic"
            Accessible.ignored: true
        }
    }
}
