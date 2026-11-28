// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard
FormCard.FormCardPage {
    id: page
    objectName: "settingsSearchPage"
    required property var appWindow
    title: i18n("Search settings")
    Kirigami.SearchField {
        id: search
        objectName: "topicSearch"
        Layout.fillWidth: true
        Layout.leftMargin: Kirigami.Units.largeSpacing
        Layout.rightMargin: Kirigami.Units.largeSpacing
        placeholderText: i18n("Battery, USB, kernel…")
        Accessible.name: i18n("Search settings")
        Component.onCompleted: forceActiveFocus()
    }
    Repeater {
        model: backend.settings.topics(-1, search.text)
        delegate: FormCard.FormCard {
            required property var modelData
            FormCard.FormButtonDelegate {
                text: modelData.title
                description: modelData.description
                icon.name: themeIcons.names[modelData.icon]
                Accessible.name: text
                onClicked: page.appWindow.openTopic(modelData.id)
            }
        }
    }
    FormCard.FormCard {
        visible: backend.settings.topics(-1, search.text).length === 0
        SafeText { text: i18n("No matching settings"); description: i18n("Try a shorter search or open the expert catalog.") }
    }
    FormCard.FormCard {
        FormCard.FormButtonDelegate {
            text: i18n("Expert action catalog")
            icon.name: themeIcons.names["utilities-terminal"]
            Accessible.name: text
            onClicked: page.appWindow.openCatalog(-1)
        }
    }
}
