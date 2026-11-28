// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

FormCard.FormCard {
    id: panel
    required property var settings
    readonly property var torch: settings.flashlight
    readonly property bool available: torch.available && !torch.pending && !settings.busy && !backend.readOnly && !backend.simulation

    FormCard.FormSwitchDelegate {
        objectName: "flashlightSwitch"
        text: i18n("Flashlight")
        checked: panel.torch.on
        enabled: panel.available
        onToggled: panel.settings.setFlashlight(checked)
    }
    FormCard.FormSectionText {
        text: i18n("Brightness")
    }
    ColumnLayout {
        Layout.fillWidth: true
        Layout.leftMargin: Kirigami.Units.largeSpacing
        Layout.rightMargin: Kirigami.Units.largeSpacing
        spacing: Kirigami.Units.smallSpacing
        Controls.ButtonGroup { id: levels }
        Repeater {
            model: [50, 100]
            delegate: Controls.RadioButton {
                required property int modelData
                objectName: "flashlightLevel" + modelData
                Layout.fillWidth: true
                Layout.minimumHeight: Kirigami.Units.gridUnit * 2
                text: modelData === 50 ? i18n("Low (%1%)", modelData) : i18n("High (%1%)", modelData)
                Controls.ButtonGroup.group: levels
                checked: panel.torch.level === modelData
                enabled: panel.available && panel.torch.twoLevels
                onClicked: panel.settings.setFlashlightBrightness(modelData)
            }
        }
    }
    FormCard.FormSectionText {
        text: panel.torch.error || (panel.settings.busy ? i18n("Reading…") :
               panel.torch.available ? i18n("Selecting a brightness level turns on the flashlight.") :
               i18n("Flashlight status is currently unavailable."))
        textFormat: Text.PlainText
    }
}
