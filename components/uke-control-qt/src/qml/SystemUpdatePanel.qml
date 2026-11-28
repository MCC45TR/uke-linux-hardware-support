// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

TileGrid {
    id: panel
    required property var settings
    required property var appWindow
    readonly property var channels: settings.actionChoices("channel.set", "channel")
    readonly property var actions: settings.topic.changes || []
    minimumTileWidth: Kirigami.Units.gridUnit * 24

    SettingsSection {
        heading: i18n("System update")
        FormCard.FormButtonDelegate {
            objectName: "updateSystem"
            text: i18n("Update system")
            description: i18n("Review signed updates for the selected channel.")
            enabled: !backend.readOnly && panel.actions.some(action => action.id === "update.apply")
            onClicked: if (backend.openAction("update.apply")) backend.submit()
        }
        FormCard.FormButtonDelegate {
            text: i18n("Preview available updates")
            enabled: panel.actions.some(action => action.id === "update.preview")
            onClicked: panel.appWindow.openGuide("update.preview")
        }
    }
    SelectionCard {
        heading: i18n("Update channel")
        choices: panel.channels
        current: panel.settings.currentChannel
        canSelect: !backend.readOnly && !panel.settings.busy && current.length > 0
        help: current ? "" : i18n("The current channel is unavailable. Refresh to check it.")
        reviewLabel: i18n("Review channel change")
        onReviewRequested: value => {
            if (backend.openAction("channel.set")) {
                backend.setDraft("channel", value)
                backend.submit()
            }
        }
    }
}
