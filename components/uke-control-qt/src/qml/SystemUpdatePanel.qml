// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

ColumnLayout {
    id: panel
    required property var settings
    readonly property var channels: settings.actionChoices("channel.set", "channel")
    readonly property var operation: settings.packageOperation
    readonly property var updateChannel: settings.updateChannel
    readonly property bool canRequest: !backend.readOnly && !backend.simulation && !operation.running && !settings.busy
    readonly property var cadence: ({
        stable: i18n("Stable is updated about once a month. Problems are rare."),
        test: i18n("Test is updated about once a week. It includes new features and can be less stable."),
        unstable: i18n("Unstable can receive daily or hourly updates. This channel is not stable.")
    })
    property var selected: ({})
    function channelTitle(value) {
        return channels.find(channel => channel.value === value)?.title || ""
    }
    spacing: Kirigami.Units.largeSpacing

    TileGrid {
        minimumTileWidth: Kirigami.Units.gridUnit * 24

        SettingsSection {
            heading: i18n("System update")
            FormCard.FormButtonDelegate {
                objectName: "updateSystem"
                text: i18n("Update system")
                enabled: panel.canRequest && panel.updateChannel.ready
                onClicked: panel.settings.runPackageOperation("update")
            }
            FormCard.FormButtonDelegate {
                text: i18n("Check for updates")
                enabled: panel.canRequest && panel.updateChannel.ready
                onClicked: panel.settings.runPackageOperation("check")
            }
            Controls.ProgressBar {
                objectName: "systemUpdateProgress"
                visible: panel.operation.running || false
                Layout.fillWidth: true
                Layout.margins: Kirigami.Units.largeSpacing
                indeterminate: visible
                Accessible.name: panel.operation.message || ""
            }
            SettingsNote {
                visible: text.length > 0
                text: panel.operation.message || ""
            }
        }

        SettingsSection {
            heading: i18n("Update channel")
            Repeater {
                model: panel.channels
                delegate: Controls.RadioButton {
                    id: channel
                    required property var modelData
                    objectName: "selection_" + modelData.value
                    Layout.fillWidth: true
                    Layout.minimumHeight: Kirigami.Units.gridUnit * 2
                    Layout.leftMargin: Kirigami.Units.largeSpacing
                    Layout.rightMargin: Kirigami.Units.largeSpacing
                    text: modelData.title
                    checked: panel.updateChannel.active === modelData.value
                    enabled: panel.canRequest
                    autoExclusive: false
                    onClicked: {
                        panel.selected = modelData
                        checked = Qt.binding(() => panel.updateChannel.active === modelData.value)
                        channelDetails.open()
                    }
                }
            }
            SettingsNote {
                objectName: "updateChannelStatus"
                visible: text.length > 0 && !panel.settings.busy
                text: {
                    const channel = panel.updateChannel
                    if (!channel.known) return i18n("The enabled software repository could not be read.")
                    if (channel.multiple) return i18n("Several Uke channels are enabled. Choose one to continue.")
                    if (!channel.active) return i18n("No Uke update channel is enabled. Choose one to continue.")
                    if (!channel.configured) return i18n("Choose a channel to save it for system updates.")
                    if (!channel.ready) return i18n("Enabled: %1. Saved: %2. Choose a channel to continue.",
                        panel.channelTitle(channel.active), panel.channelTitle(channel.configured))
                    return ""
                }
            }
        }
    }

    SettingsSection {
        visible: (panel.operation.packages || []).length > 0
        heading: i18n("Pending updates")
        Repeater {
            model: panel.operation.packages || []
            delegate: SafeText {
                required property string modelData
                text: modelData
            }
        }
    }

    readonly property FormCard.FormCardDialog channelDetails: FormCard.FormCardDialog {
        parent: Controls.Overlay.overlay
        title: panel.selected.title || ""
        width: Math.min(parent.width - Kirigami.Units.largeSpacing * 2, Kirigami.Units.gridUnit * 27)
        standardButtons: Controls.Dialog.Cancel
        SettingsNote {
            text: panel.cadence[panel.selected.value] || ""
        }
        FormCard.FormButtonDelegate {
            text: i18n("Change channel")
            enabled: panel.canRequest && (!panel.updateChannel.ready || panel.selected.value !== panel.updateChannel.active)
            onClicked: {
                channelDetails.close()
                panel.settings.runPackageOperation("channel", panel.selected.value)
            }
        }
    }
}
