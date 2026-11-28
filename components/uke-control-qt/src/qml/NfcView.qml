// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

TileGrid {
    id: view
    readonly property var reader: backend.settings.nfc
    readonly property var reading: reader.report
    minimumTileWidth: Kirigami.Units.gridUnit * 24

    SettingsSection {
        heading: i18n("Read an NFC tag")
        Kirigami.Icon {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: Kirigami.Units.largeSpacing
            Layout.preferredWidth: Kirigami.Units.iconSizes.huge
            Layout.preferredHeight: Layout.preferredWidth
            source: "network-wireless"
            Accessible.ignored: true
        }
        SettingsNote {
            text: view.reading.message || i18n("Connect an external NFC reader and hold a tag near it.")
        }
        Controls.BusyIndicator {
            visible: running
            running: view.reading.busy || false
            Layout.alignment: Qt.AlignHCenter
        }
        FormCard.FormButtonDelegate {
            objectName: "nfcStart"
            text: i18n("Start reading")
            icon.name: "media-playback-start"
            enabled: !backend.simulation && !view.reading.busy && (view.reading.supported || false)
            onClicked: view.reader.start()
        }
        FormCard.FormButtonDelegate {
            objectName: "nfcInspect"
            text: i18n("Check reader")
            icon.name: "view-refresh"
            visible: !view.reading.supported
            enabled: !backend.simulation && !view.reading.busy
            onClicked: view.reader.inspect()
        }
        FormCard.FormButtonDelegate {
            text: i18n("Start NFC reader service")
            icon.name: "system-run"
            visible: !view.reading.supported && !backend.readOnly && !backend.simulation
            enabled: !view.reading.busy
            onClicked: view.reader.startService()
        }
        FormCard.FormButtonDelegate {
            text: i18n("Stop reading")
            icon.name: "media-playback-stop"
            visible: (view.reading.busy && !view.reading.startingService) || false
            onClicked: view.reader.stop()
        }
    }

    SettingsSection {
        heading: i18n("Tag contents")
        visible: (view.reading.records || []).length > 0
        Repeater {
            model: view.reading.records || []
            delegate: SafeText {
                required property var modelData
                text: modelData.title
                description: modelData.value
            }
        }
        FormCard.FormButtonDelegate {
            text: i18n("Clear read content")
            icon.name: "edit-clear"
            onClicked: view.reader.clear()
        }
    }
}
