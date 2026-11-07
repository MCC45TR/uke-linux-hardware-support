// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

FormCard.FormCardPage {
    id: page
    objectName: "areaPage"
    required property var appWindow
    readonly property var area: backend.areas[backend.section]
    title: area.title
    actions: [
        Kirigami.Action {
            text: "Find an action"
            icon.name: themeIcons.names["edit-find"]
            onTriggered: page.appWindow.openCatalog()
        }
    ]
    Kirigami.InlineMessage {
        Layout.fillWidth: true
        visible: backend.simulation || backend.readOnly
        type: backend.simulation ? Kirigami.MessageType.Warning : Kirigami.MessageType.Information
        text: backend.simulation ? "Simulation — sample data only. Device commands and changes are disabled." : "Read-only session. Changes are disabled."
    }
    Kirigami.InlineMessage {
        Layout.fillWidth: true
        visible: styleNotice.length > 0
        type: Kirigami.MessageType.Warning
        text: styleNotice
    }
    SafeText {
        icon.name: themeIcons.names[page.area.icon]
        text: page.area.caption
        description: page.area.detail
    }
    FormCard.FormHeader {
        visible: backend.section === 0
        title: "Browse your device"
        maximumWidth: Kirigami.Units.gridUnit * 58
    }
    Kirigami.CardsLayout {
        objectName: "categoryCards"
        Layout.fillWidth: true
        Layout.maximumWidth: Kirigami.Units.gridUnit * 58
        Layout.alignment: Qt.AlignHCenter
        Layout.leftMargin: Kirigami.Units.largeSpacing
        Layout.rightMargin: Kirigami.Units.largeSpacing
        visible: backend.section === 0
        maximumColumns: 2
        minimumColumnWidth: Kirigami.Units.gridUnit * 18
        maximumColumnWidth: Kirigami.Units.gridUnit * 28
        Repeater {
            model: backend.section === 0 ? backend.areas.slice(1) : []
            delegate: Kirigami.AbstractCard {
                required property var modelData
                objectName: "category_" + modelData.id
                Layout.fillWidth: true
                showClickFeedback: true
                Accessible.name: modelData.title
                Accessible.description: modelData.description
                onClicked: page.appWindow.showArea(modelData.index)
                contentItem: ColumnLayout {
                    spacing: Kirigami.Units.smallSpacing
                    RowLayout {
                        Layout.fillWidth: true
                        Kirigami.Icon {
                            source: themeIcons.names[modelData.icon]
                            implicitWidth: Kirigami.Units.iconSizes.medium
                            implicitHeight: implicitWidth
                            Accessible.ignored: true
                        }
                        Kirigami.Heading { Layout.fillWidth: true; level: 4; text: modelData.title; wrapMode: Text.WordWrap }
                        Kirigami.Icon {
                            source: themeIcons.names["go-next"]
                            implicitWidth: Kirigami.Units.iconSizes.small
                            implicitHeight: implicitWidth
                            Accessible.ignored: true
                        }
                    }
                    Controls.Label {
                        Layout.fillWidth: true
                        text: modelData.caption
                        wrapMode: Text.WordWrap
                        color: Kirigami.Theme.disabledTextColor
                    }
                }
            }
        }
    }
    Repeater {
        model: backend.groups
        delegate: ColumnLayout {
            id: groupColumn
            required property var modelData
            Layout.fillWidth: true
            spacing: 0
            FormCard.FormHeader { title: groupColumn.modelData.title }
            FormCard.FormCard {
                Layout.fillWidth: true
                FormCard.FormSectionText { text: groupColumn.modelData.description; textFormat: Text.PlainText }
                Repeater {
                    model: groupColumn.modelData.entries
                    delegate: ActionRow {
                        required property var modelData
                        entry: modelData
                        onClicked: page.appWindow.openGuide(entry.id)
                    }
                }
            }
        }
    }
    FormCard.FormHeader { title: "Explore further" }
    FormCard.FormCard {
        FormCard.FormButtonDelegate {
            objectName: "browseAllActions"
            text: backend.section === 0 ? "Search all actions" : "More tools in this category"
            description: backend.section === 0 ? "Search by task, category or action type. Shortcut: Ctrl+K." : "Find additional status checks, preferences and diagnostics."
            Accessible.name: text
            icon.name: themeIcons.names["edit-find"]
            onClicked: page.appWindow.openCatalog(backend.section === 0 ? -1 : backend.section)
        }
    }
}
