// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

SettingsSection {
    id: panel
    required property var settings
    readonly property var reading: settings.infrared
    heading: i18n("NEC test signal")
    SettingsNote {
        text: panel.reading.available
              ? i18n("Aim the tablet at the receiver and enter its four-digit hexadecimal NEC code.")
              : i18n("A supported infrared transmitter was not found.")
    }
    Controls.TextField {
        id: code
        objectName: "infraredCode"
        Layout.fillWidth: true
        Layout.margins: Kirigami.Units.largeSpacing
        placeholderText: i18n("NEC code")
        Accessible.name: placeholderText
        enabled: panel.reading.available && !panel.reading.busy
        maximumLength: 4
        validator: RegularExpressionValidator { regularExpression: /^[0-9A-Fa-f]{4}$/ }
    }
    FormCard.FormButtonDelegate {
        objectName: "sendInfrared"
        text: i18n("Send test signal")
        enabled: panel.reading.available && !panel.reading.busy && code.acceptableInput && !backend.readOnly && !backend.simulation
        onClicked: panel.settings.sendInfrared(code.text)
    }
    Kirigami.InlineMessage {
        Layout.fillWidth: true
        Layout.margins: Kirigami.Units.largeSpacing
        visible: panel.reading.message.length > 0
        type: panel.reading.error ? Kirigami.MessageType.Error : Kirigami.MessageType.Positive
        text: panel.reading.message
    }
}
