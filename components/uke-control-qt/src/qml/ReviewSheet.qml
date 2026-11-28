// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

FormCard.FormCardDialog {
    id: dialog
    objectName: "requestReview"
    title: backend.review.read ? i18n("Review read access") : i18n("Review request")
    width: Math.min(parent.width - Kirigami.Units.largeSpacing * 2, Kirigami.Units.gridUnit * 32)
    height: Math.min(implicitHeight, parent.height - Kirigami.Units.largeSpacing * 2)
    standardButtons: Controls.Dialog.Cancel
    onOpened: { consent.checked = false; confirmation.text = "" }
    onRejected: backend.cancelReview()
    SafeText {
        text: backend.review.title || ""
        description: backend.review.description || ""
    }
    SafeText {
        text: backend.review.role || ""
        description: backend.review.access || ""
        icon.name: themeIcons.names["security-high"]
    }
    Repeater {
        model: backend.review.values || []
        delegate: SafeText {
            required property var modelData
            text: modelData.title
            description: modelData.value
        }
    }
    SafeText {
        visible: (backend.review.warning || "").length > 0
        icon.name: themeIcons.names["dialog-warning"]
        text: i18n("Access & effects")
        description: backend.review.warning || ""
    }
    FormCard.FormTextFieldDelegate {
        id: confirmation
        objectName: "confirmationToken"
        visible: (backend.review.token || "").length > 0
        label: i18n("Type %1 to confirm", backend.review.token || "")
        Accessible.name: label
    }
    FormCard.FormCheckDelegate {
        id: consent
        objectName: "reviewConsent"
        text: backend.review.read ? i18n("I reviewed why terminal access is needed.") : i18n("I reviewed the request and its effects.")
        Accessible.name: text
    }
    SafeText {
        visible: backend.review.blocked || false
        icon.name: themeIcons.names["security-high"]
        text: i18n("Action disabled")
        description: backend.review.reason || ""
    }
    FormCard.FormButtonDelegate {
            Accessible.name: text
        text: i18n("Command details")
        icon.name: themeIcons.names["document-properties"]
        Accessible.description: exact.visible ? "Arguments expanded" : "Arguments collapsed"
        onClicked: exact.visible = !exact.visible
    }
    SafeText {
        id: exact
        visible: false
        text: i18n("Validated command")
        description: backend.review.command || ""
    }
    FormCard.FormButtonDelegate {
            Accessible.name: text
        objectName: "launchTerminal"
        text: i18n("Continue in terminal")
        description: i18n("The terminal requests any required authentication and shows the operation's result.")
        icon.name: themeIcons.names["utilities-terminal"]
        enabled: !(backend.review.blocked || false) && consent.checked && confirmation.text === (backend.review.token || "")
        onClicked: backend.confirmReview(consent.checked, confirmation.text)
    }
}
