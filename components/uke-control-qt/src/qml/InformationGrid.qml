// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

TileGrid {
    id: grid
    required property var sections
    property var expandedKeys: []
    property bool stretchTiles: false

    Layout.maximumWidth: Kirigami.Units.gridUnit * (sections.length > 1 ? 48 : 30)
    Layout.alignment: Qt.AlignHCenter
    Layout.margins: Kirigami.Units.largeSpacing
    minimumTileWidth: Kirigami.Units.gridUnit * 21
    visible: sections.length > 0

    Repeater {
        model: grid.sections
        delegate: InformationCard {
            required property var modelData
            section: modelData
            objectName: "informationCard_" + section.key
            expanded: grid.expandedKeys.includes(section.key)
            Layout.fillHeight: grid.stretchTiles && grid.columns > 1 && grid.expandedKeys.length === 0
            onToggleDetails: {
                const key = section.key
                grid.expandedKeys = expanded ? grid.expandedKeys.filter(item => item !== key)
                                             : grid.expandedKeys.concat(key)
            }
        }
    }
}
