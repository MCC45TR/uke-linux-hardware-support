// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

GridLayout {
    property int maximumColumns: 0
    property real minimumTileWidth: Kirigami.Units.gridUnit * 16
    readonly property int fittingColumns: Math.max(1,
        Math.floor((width + columnSpacing) / (minimumTileWidth + columnSpacing)))

    Layout.fillWidth: true
    Layout.minimumWidth: 0
    columns: maximumColumns > 0 ? Math.min(maximumColumns, fittingColumns) : fittingColumns
    uniformCellWidths: true
    columnSpacing: Kirigami.Units.largeSpacing
    rowSpacing: Kirigami.Units.largeSpacing
}
