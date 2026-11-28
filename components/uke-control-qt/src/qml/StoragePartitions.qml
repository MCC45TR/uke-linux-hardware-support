// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

TileGrid {
    id: panel
    required property var settings
    readonly property var partitions: settings.linuxPartitions
    readonly property var rootVolume: partitions.find(part => part.root)
    minimumTileWidth: Kirigami.Units.gridUnit * 24

    SettingsSection {
        heading: i18n("Linux partitions")
        SafeText {
            visible: panel.partitions.length === 0
            text: panel.settings.busy ? i18n("Reading…") : i18n("No Linux partitions could be identified.")
        }
        SafeText {
            visible: panel.partitions.some(part => part.stale)
            text: i18n("Showing the last available reading.")
        }
        Repeater {
            model: panel.partitions
            delegate: SafeText {
                required property var modelData
                text: modelData.title
                description: [modelData.device, modelData.size, modelData.filesystem].filter(Boolean).join(" · ")
            }
        }
    }
    SettingsSection {
        heading: i18n("Storage encryption")
        FormCard.FormButtonDelegate {
            objectName: "storageEncryption"
            text: i18n("Encryption options")
            description: !panel.rootVolume ? i18n("Root encryption status is unavailable.") :
                         panel.rootVolume.encrypted ? i18n("The running system uses LUKS.") :
                         i18n("The running system has no reported LUKS layer.")
            icon.name: themeIcons.names["security-high"]
            onClicked: encryption.open()
        }
    }
    readonly property FormCard.FormCardDialog encryption: FormCard.FormCardDialog {
        parent: Controls.Overlay.overlay
        title: i18n("Storage encryption")
        width: Math.min(parent.width - Kirigami.Units.largeSpacing * 2, Kirigami.Units.gridUnit * 30)
        standardButtons: Controls.Dialog.Close
        SafeText {
            text: panel.rootVolume ? panel.rootVolume.device : i18n("Running Linux system")
            description: panel.rootVolume && panel.rootVolume.encrypted
                         ? i18n("This volume already uses LUKS. Keep an independent recovery key and a current backup.")
                         : i18n("Encrypting this system volume requires a supported offline migration and recovery setup. This installation has no verified conversion path.")
        }
    }
}
