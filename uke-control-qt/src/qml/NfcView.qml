// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard
ColumnLayout {
    readonly property var reader: backend.settings.nfc
    Layout.fillWidth: true
    spacing: 0
    FormCard.FormHeader { title: i18n("Read an NFC tag") }
    FormCard.FormCard {
        Layout.fillWidth: true
        SafeText { text: i18n("Connected NFC reader"); description: reader.report.message || "" }
        FormCard.FormButtonDelegate {
            objectName: "nfcInspect"
            text: i18n("Check reader")
            icon.name: themeIcons.names["view-refresh"]
            enabled: !backend.simulation && !reader.report.busy
            onClicked: reader.inspect()
        }
        FormCard.FormButtonDelegate {
            text: i18n("Start NFC reader service")
            description: i18n("Start the distribution neard service. Administrator authorization may be requested.")
            icon.name: themeIcons.names["system-run"]
            enabled: !backend.simulation && !reader.report.busy
            onClicked: reader.startService()
        }
        FormCard.FormButtonDelegate {
            objectName: "nfcStart"
            text: i18n("Read NDEF tag")
            description: i18n("Use a supported external reader. No card authentication, payment or memory writes are performed.")
            icon.name: themeIcons.names["network-wireless"]
            enabled: !backend.simulation && !reader.report.busy && reader.report.supported
            onClicked: reader.start()
        }
        FormCard.FormButtonDelegate {
            text: i18n("Stop reading")
            icon.name: themeIcons.names["media-playback-stop"]
            visible: (reader.report.busy && !reader.report.startingService) || false
            onClicked: reader.stop()
        }
        Repeater {
            model: reader.report.records || []
            delegate: SafeText {
                required property var modelData
                text: modelData.title
                description: modelData.value
            }
        }
        FormCard.FormButtonDelegate {
            text: i18n("Clear read content")
            visible: (reader.report.records || []).length > 0
            icon.name: themeIcons.names["edit-clear"]
            onClicked: reader.clear()
        }
    }
}
