// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

RowLayout {
    id: tiles
    required property var items
    required property Component tile
    property int maximumColumns: 0
    property real minimumTileWidth: Kirigami.Units.gridUnit * 16
    readonly property int fittingColumns: Math.max(1,
        Math.floor((width + spacing) / (minimumTileWidth + spacing)))
    readonly property int columns: Math.max(1, Math.min(items.length, fittingColumns,
        maximumColumns > 0 ? maximumColumns : fittingColumns))

    Layout.fillWidth: true
    Layout.minimumWidth: 0
    uniformCellSizes: true
    spacing: Kirigami.Units.largeSpacing

    Repeater {
        model: tiles.columns
        delegate: ColumnLayout {
            id: column
            required property int index
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            Layout.alignment: Qt.AlignTop
            spacing: tiles.spacing

            Repeater {
                model: Math.ceil((tiles.items.length - column.index) / tiles.columns)
                delegate: Loader {
                    required property int index
                    readonly property var tileData: tiles.items[index * tiles.columns + column.index]
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    active: tileData !== undefined
                    sourceComponent: tiles.tile
                }
            }
        }
    }
}
