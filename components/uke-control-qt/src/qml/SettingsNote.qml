// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

FormCard.FormSectionText {
    Layout.maximumWidth: Number.POSITIVE_INFINITY
    color: Kirigami.Theme.textColor
    textFormat: Text.PlainText
}
