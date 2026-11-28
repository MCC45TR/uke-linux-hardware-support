// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

ColumnLayout {
    id: panel
    required property var settings
    required property var appWindow
    readonly property var channels: settings.actionChoices("channel.set", "channel")
    readonly property var operation: settings.packageOperation
    readonly property var updateChannel: settings.updateChannel
    readonly property bool canRequest: !backend.readOnly && !backend.simulation && !operation.running && !settings.busy
    readonly property var cadence: ({
        stable: i18n("Stable is updated about once a month. Problems are rare."),
        test: i18n("Test is updated about once a week. It includes new features and can be less stable."),
        unstable: i18n("Unstable can receive daily or hourly updates. This channel is not stable.")
    })
    property string proposedChannel: ""
    function channelTitle(value) {
        return channels.find(channel => channel.value === value)?.title || ""
    }

    spacing: Kirigami.Units.largeSpacing

    SettingsSection {
        heading: i18n("Update channel")

        TileGrid {
            maximumColumns: 3
            minimumTileWidth: Kirigami.Units.gridUnit * 16
            Layout.margins: Kirigami.Units.largeSpacing
            Controls.ButtonGroup { id: channelButtons }

            Repeater {
                model: panel.channels
                delegate: ColumnLayout {
                    id: option
                    required property var modelData
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    Layout.fillHeight: true
                    spacing: Kirigami.Units.mediumSpacing
                    Controls.Label {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.alignment: Qt.AlignTop
                        text: panel.cadence[option.modelData.value] || ""
                        wrapMode: Text.Wrap
                    }
                    Controls.RadioButton {
                        objectName: "selection_" + option.modelData.value
                        Layout.fillWidth: true
                        Layout.minimumHeight: Kirigami.Units.gridUnit * 2
                        Controls.ButtonGroup.group: channelButtons
                        text: option.modelData.title
                        checked: (panel.proposedChannel || panel.updateChannel.active) === option.modelData.value
                        enabled: panel.canRequest
                        onClicked: panel.proposedChannel = option.modelData.value
                    }
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
                return i18n("Enabled repository: %1", panel.channelTitle(channel.active))
            }
        }
        SettingsNote {
            visible: text.length > 0 && !panel.settings.busy
            text: panel.updateChannel.kernel ? i18n("Running kernel: %1", panel.updateChannel.kernel) : ""
        }
        FormCard.FormButtonDelegate {
            objectName: "applyUpdateChannel"
            text: i18n("Change channel")
            description: i18n("Changes the software repository. Updates are installed when you choose Update system.")
            enabled: panel.canRequest && panel.proposedChannel.length > 0
                     && (!panel.updateChannel.ready || panel.proposedChannel !== panel.updateChannel.active)
            onClicked: {
                const channel = panel.proposedChannel
                panel.proposedChannel = ""
                panel.settings.runPackageOperation("channel", channel)
            }
        }
    }

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
        SettingsNote {
            visible: text.length > 0
            text: panel.operation.detail || ""
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

    SettingsSection {
        FormCard.FormButtonDelegate {
            objectName: "openChangelog"
            text: i18n("What’s new")
            description: i18n("Read the highlights from recent Uke releases.")
            icon.name: "document-preview"
            onClicked: panel.appWindow.openTopic("changelog")
        }
    }
}
