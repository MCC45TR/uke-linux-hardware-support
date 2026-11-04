// SPDX-License-Identifier: MIT
import QtQuick
import org.kde.kirigamiaddons.formcard as FormCard

FormCard.FormButtonDelegate {
    required property var entry
    objectName: "action_" + entry.id
    text: entry.title
    description: entry.description + "\n" + entry.role + (entry.access === "Administrator access" ? " · Administrator access" : "")
    icon.name: themeIcons.names[entry.icon]
    Accessible.name: text
    Accessible.description: description
}
