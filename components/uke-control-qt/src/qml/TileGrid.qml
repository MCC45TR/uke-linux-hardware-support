// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

GridLayout {
    property int maximumColumns: 3
    property real minimumTileWidth: Kirigami.Units.gridUnit * 16

    Layout.fillWidth: true
    Layout.minimumWidth: 0
    columns: Math.max(1, Math.min(maximumColumns,
        Math.floor((width + columnSpacing) / (minimumTileWidth + columnSpacing))))
    uniformCellWidths: true
    columnSpacing: Kirigami.Units.largeSpacing
    rowSpacing: Kirigami.Units.largeSpacing
}
