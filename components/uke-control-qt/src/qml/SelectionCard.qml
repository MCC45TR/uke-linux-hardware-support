// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

SettingsSection {
    id: panel
    required property var choices
    required property string current
    property bool canSelect: true
    property string help: ""
    property string reviewLabel: i18n("Review change")
    property string selected: ""
    signal reviewRequested(string selection)

    onCurrentChanged: selected = ""
    onCanSelectChanged: if (!canSelect) selected = ""

    Controls.ButtonGroup { id: selection }
    TileGrid {
        Layout.margins: Kirigami.Units.largeSpacing
        minimumTileWidth: Kirigami.Units.gridUnit * 11
        Repeater {
            model: panel.choices
            delegate: Controls.RadioButton {
                required property var modelData
                objectName: "selection_" + modelData.value
                Layout.fillWidth: true
                Layout.minimumHeight: Kirigami.Units.gridUnit * 2
                text: modelData.title
                checked: (panel.selected || panel.current) === modelData.value
                enabled: panel.canSelect && modelData.enabled !== false
                Controls.ButtonGroup.group: selection
                onClicked: panel.selected = modelData.value
            }
        }
    }
    SettingsNote {
        visible: panel.help.length > 0
        text: panel.help
    }
    FormCard.FormButtonDelegate {
        text: panel.reviewLabel
        enabled: panel.canSelect && panel.selected.length > 0 && panel.selected !== panel.current
        onClicked: panel.reviewRequested(panel.selected)
    }
}
