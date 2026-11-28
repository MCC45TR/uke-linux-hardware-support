// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

FormCard.FormCardPage {
    id: page
    objectName: "settingsSearchPage"
    required property var appWindow
    readonly property var matches: backend.settings.topics(-1, search.text)

    title: i18n("Search settings")
    function focusSearch() { search.forceActiveFocus() }
    Component.onCompleted: Qt.callLater(focusSearch)

    actions: [Kirigami.Action {
        text: i18n("Expert action catalog")
        icon.name: themeIcons.names["utilities-terminal"]
        onTriggered: page.appWindow.openCatalog(-1)
    }]

    ColumnLayout {
        Layout.fillWidth: true
        Layout.maximumWidth: Kirigami.Units.gridUnit * 68
        Layout.alignment: Qt.AlignHCenter
        Layout.margins: Kirigami.Units.largeSpacing
        spacing: Kirigami.Units.largeSpacing

        Kirigami.SearchField {
            id: search
            objectName: "topicSearch"
            Layout.fillWidth: true
            placeholderText: i18n("Battery, USB, kernel…")
            Accessible.name: i18n("Search settings")
        }

        TileGrid {
            visible: page.matches.length > 0
            Repeater {
                model: page.matches
                delegate: SettingsTile {
                    required property var modelData
                    text: modelData.title
                    description: modelData.description
                    icon.name: themeIcons.names[modelData.icon]
                    onClicked: page.appWindow.openTopic(modelData.id)
                }
            }
        }

        Kirigami.PlaceholderMessage {
            Layout.fillWidth: true
            visible: page.matches.length === 0
            text: i18n("No matching settings")
            explanation: i18n("Try a shorter search or open the expert catalog.")
        }
    }
}
