// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.AbstractCard {
    Layout.fillWidth: true
    Layout.minimumWidth: 0
    padding: Kirigami.Units.largeSpacing * 2
    showClickFeedback: false
    focusPolicy: Qt.NoFocus

    contentItem: ColumnLayout {
        spacing: Kirigami.Units.mediumSpacing
        Kirigami.Icon {
            Layout.preferredWidth: Kirigami.Units.iconSizes.enormous
            Layout.preferredHeight: Layout.preferredWidth
            source: "org.uke.ControlQt"
            Accessible.ignored: true
        }
        Kirigami.Heading { Layout.fillWidth: true; level: 1; text: i18n("Uke Control") }
        Controls.Label { Layout.fillWidth: true; text: i18n("Version %1", Qt.application.version); wrapMode: Text.Wrap }
        Controls.Label {
            Layout.fillWidth: true
            text: i18n("Manage Uke hardware, battery, connected devices and system updates from Plasma.")
            wrapMode: Text.Wrap
        }
        Controls.Label { Layout.fillWidth: true; text: i18n("License: MIT"); wrapMode: Text.Wrap }
    }
}
