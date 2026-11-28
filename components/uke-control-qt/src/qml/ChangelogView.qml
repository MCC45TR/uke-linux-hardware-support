// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

ColumnLayout {
    id: view
    required property var settings
    readonly property var changelog: settings.changelog
    spacing: Kirigami.Units.largeSpacing

    Kirigami.InlineMessage {
        Layout.fillWidth: true
        visible: view.changelog.message.length > 0
        text: view.changelog.message
    }
    InformationGrid {
        Layout.maximumWidth: Number.POSITIVE_INFINITY
        Layout.margins: 0
        minimumTileWidth: Kirigami.Units.gridUnit * 21
        sections: view.changelog.entries
    }
}
