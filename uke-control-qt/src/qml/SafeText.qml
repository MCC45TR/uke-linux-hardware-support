// SPDX-License-Identifier: MIT
import QtQuick
import org.kde.kirigamiaddons.formcard as FormCard

FormCard.FormTextDelegate {
    Accessible.role: Accessible.StaticText
    Accessible.name: text
    Accessible.description: description
    textItem.textFormat: Text.PlainText
    descriptionItem.textFormat: Text.PlainText
}
