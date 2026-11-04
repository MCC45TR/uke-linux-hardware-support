// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kirigamiaddons.formcard as FormCard
ColumnLayout {
    required property var topicPage
    Layout.fillWidth: true
    spacing: 0
    FormCard.FormHeader { title: i18n("Host mode and automatic handling") }
    FormCard.FormCard {
        Layout.fillWidth: true
        SafeText {
            text: backend.settings.usbPolicy.known && backend.settings.usbPolicy.automatic ? i18n("Automatic handling is active") : i18n("Automatic handling has not been verified")
            description: backend.settings.usbPolicy.message
        }
        FormCard.FormButtonDelegate {
            objectName: "usbAutomatic"
            text: i18n("Automatic")
            description: i18n("Check kernel management and current roles. Keeps the attached connection intact.")
            icon.name: themeIcons.names["view-refresh"]
            enabled: !backend.settings.busy
            onClicked: backend.settings.refresh()
        }
        FormCard.FormButtonDelegate {
            objectName: "usbRequestRole"
            text: i18n("Request Host or Device")
            description: i18n("A one-time role request. Active dock storage, Ethernet and displays can prevent a change.")
            icon.name: themeIcons.names["configure"]
            enabled: !backend.readOnly
            onClicked: topicPage.edit("usb.role")
        }
    }
}
