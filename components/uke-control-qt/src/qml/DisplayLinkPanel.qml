// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

FormCard.FormCard {
    id: panel
    required property var settings
    required property var appWindow
    readonly property var link: settings.displayLink
    readonly property var actions: settings.topic.changes || []

    SafeText {
        text: i18n("DisplayPort link speed")
        description: panel.link.rate && panel.link.lanes > 0
                     ? i18n("%1 · %2 lanes · %3 bits per pixel", panel.link.rate, panel.link.lanes, panel.link.bitsPerPixel)
                     : i18n("No negotiated link speed reported")
    }
    FormCard.FormSectionText {
        text: panel.link.stale ? i18n("Showing the last available reading.") :
              i18n("The driver negotiates the link speed automatically. This driver has no manual HBR2 or HBR3 selector.")
        textFormat: Text.PlainText
    }
    ColumnLayout {
        Layout.fillWidth: true
        Layout.leftMargin: Kirigami.Units.largeSpacing
        Layout.rightMargin: Kirigami.Units.largeSpacing
        spacing: Kirigami.Units.smallSpacing
        Controls.RadioButton {
            Layout.fillWidth: true
            text: i18n("Automatic")
            checked: true
            enabled: false
        }
        Controls.RadioButton { Layout.fillWidth: true; text: "HBR2"; enabled: false }
        Controls.RadioButton { Layout.fillWidth: true; text: "HBR3"; enabled: false }
    }
    FormCard.FormButtonDelegate {
        visible: panel.actions.some(action => action.id === "display.lanes")
        enabled: (panel.link.canSetLanes || false) && !backend.readOnly
        text: i18n("DisplayPort lane limit")
        description: panel.link.ceiling ? i18n("Current ceiling: %1 lanes. Disconnect the display before applying a change.", panel.link.ceiling)
                                         : i18n("Disconnect the display before applying a change.")
        onClicked: panel.appWindow.openGuide("display.lanes")
    }
    FormCard.FormButtonDelegate {
        visible: panel.actions.some(action => action.id === "display.compression-set")
        enabled: (panel.link.canSetCompression || false) && !backend.readOnly
        text: i18n("Display stream compression")
        description: i18n("Review a compression change before applying it with the display disconnected.")
        onClicked: panel.appWindow.openGuide("display.compression-set")
    }
}
