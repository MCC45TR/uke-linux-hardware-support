// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Item {
    id: tiles
    required property var items
    required property Component tile
    property int maximumColumns: 0
    property real tileHeight: -1
    property real spacing: Kirigami.Units.largeSpacing
    property real minimumTileWidth: Kirigami.Units.gridUnit * 16
    readonly property int fittingColumns: Math.max(1,
        Math.floor((width + spacing) / (minimumTileWidth + spacing)))
    readonly property int columns: Math.max(1, Math.min(items.length, fittingColumns,
        maximumColumns > 0 ? maximumColumns : fittingColumns))
    readonly property real columnWidth: Math.max(0, (width - spacing * (columns - 1)) / columns)

    Layout.fillWidth: true
    Layout.minimumWidth: 0
    implicitWidth: minimumTileWidth
    implicitHeight: childrenRect.height

    Repeater {
        model: tiles.columns
        delegate: ColumnLayout {
            id: column
            required property int index
            x: (tiles.LayoutMirroring.enabled ? tiles.columns - 1 - index : index)
               * (tiles.columnWidth + tiles.spacing)
            width: tiles.columnWidth
            height: implicitHeight
            spacing: tiles.spacing

            Repeater {
                model: Math.ceil((tiles.items.length - column.index) / tiles.columns)
                delegate: Loader {
                    required property int index
                    readonly property var tileData: tiles.items[index * tiles.columns + column.index]
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    Layout.preferredHeight: tiles.tileHeight
                    active: tileData !== undefined
                    sourceComponent: tiles.tile
                }
            }
        }
    }
}
