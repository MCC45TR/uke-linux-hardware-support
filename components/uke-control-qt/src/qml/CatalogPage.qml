// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

FormCard.FormCardPage {
    id: page
    objectName: "catalogPage"
    required property var appWindow
    property int initialArea: -1
    readonly property var filteredGroups: backend.catalog(search.text, category.currentValue === undefined ? -1 : category.currentValue, kind.currentValue || "all")
    readonly property int resultCount: {
        let count = 0
        for (let group of filteredGroups) count += group.entries.length
        return count
    }
    title: i18n("All actions")
    function focusSearch() { search.forceActiveFocus() }
    Component.onCompleted: { if (initialArea < 0) Qt.callLater(focusSearch) }
    SafeText {
        text: i18n("Find a task")
        description: i18n("Search titles, descriptions or action identifiers. Filter by category and the kind of work you want to do.")
        icon.name: themeIcons.names["edit-find"]
    }
    Kirigami.SearchField {
        id: search
        objectName: "advancedSearch"
        Layout.fillWidth: true
        Layout.maximumWidth: Kirigami.Units.gridUnit * 30
        Layout.alignment: Qt.AlignHCenter
        Layout.leftMargin: Kirigami.Units.largeSpacing
        Layout.rightMargin: Kirigami.Units.largeSpacing
        placeholderText: i18n("Search actions…")
        Accessible.name: i18n("Search all actions")
    }
    FormCard.FormCard {
        FormCard.FormComboBoxDelegate {
            id: category
            objectName: "catalogCategory"
            text: i18n("Category")
            Accessible.name: text
            model: [{title: "All categories", value: -1}].concat(backend.areas.map(area => ({title: area.title, value: area.index})))
            textRole: "title"
            valueRole: "value"
            currentIndex: page.initialArea + 1
            displayMode: page.width < Kirigami.Units.gridUnit * 35 ? FormCard.FormComboBoxDelegate.Dialog : FormCard.FormComboBoxDelegate.ComboBox
        }
        FormCard.FormComboBoxDelegate {
            id: kind
            objectName: "catalogKind"
            text: i18n("Action type")
            Accessible.name: text
            model: [
                {title: "All types", value: "all"},
                {title: "Read information", value: "read"},
                {title: "Change preferences & software", value: "change"},
                {title: "Files & sharing", value: "files"},
                {title: "Maintenance", value: "maintenance"}
            ]
            textRole: "title"
            valueRole: "value"
            currentIndex: 0
            displayMode: category.displayMode
        }
    }
    FormCard.FormHeader { title: page.resultCount + (page.resultCount === 1 ? " matching action" : " matching actions") }
    FormCard.FormCard {
        visible: page.resultCount === 0
        SafeText {
            text: i18n("No matching actions")
            description: i18n("Try a shorter phrase or reset the filters.")
            icon.name: themeIcons.names["edit-find"]
        }
        FormCard.FormButtonDelegate {
            objectName: "resetCatalog"
            text: i18n("Reset search & filters")
            icon.name: themeIcons.names["edit-clear"]
            Accessible.name: text
            onClicked: { search.text = ""; category.currentIndex = 0; kind.currentIndex = 0; search.forceActiveFocus() }
        }
    }
    Repeater {
        model: page.filteredGroups
        delegate: ColumnLayout {
            id: resultGroup
            required property var modelData
            Layout.fillWidth: true
            spacing: 0
            FormCard.FormHeader { title: resultGroup.modelData.title }
            FormCard.FormCard {
                Repeater {
                    model: resultGroup.modelData.entries
                    delegate: ActionRow {
                        required property var modelData
                        entry: modelData
                        onClicked: page.appWindow.openGuide(entry.id)
                    }
                }
            }
        }
    }
}
