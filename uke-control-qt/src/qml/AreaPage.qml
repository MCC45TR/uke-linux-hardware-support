// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard
FormCard.FormCardPage {
    id: page
    objectName: "areaPage"
    required property var appWindow
    property int areaIndex: 0
    readonly property var area: backend.areas[areaIndex]
    title: area.title
    actions: [Kirigami.Action {
        text: i18n("Search settings")
        icon.name: themeIcons.names["edit-find"]
        onTriggered: page.appWindow.openSearch()
    }]
    Kirigami.InlineMessage {
        Layout.fillWidth: true
        visible: backend.simulation || backend.readOnly
        type: Kirigami.MessageType.Information
        text: backend.simulation ? i18n("Simulation — sample data only. Changes are disabled.") : i18n("Read-only session")
    }
    Repeater {
        model: page.areaIndex === 0 ? backend.areas.filter(a => a.index >= 1 && a.index <= 6) : []
        delegate: FormCard.FormCard {
            required property var modelData
            Layout.fillWidth: true
            FormCard.FormButtonDelegate {
                objectName: "category_" + modelData.id
                text: modelData.title
                description: { let changed = backend.settings.updated; return backend.settings.summary(modelData.index) || modelData.caption }
                icon.name: themeIcons.names[modelData.icon]
                Accessible.name: text
                onClicked: page.appWindow.showArea(modelData.index)
            }
        }
    }
    FormCard.FormHeader { visible: page.areaIndex === 0; title: i18n("More") }
    FormCard.FormCard {
        visible: page.areaIndex === 0
        Repeater {
            model: backend.areas.filter(a => a.secondary)
            delegate: FormCard.FormButtonDelegate {
                required property var modelData
                text: modelData.title
                description: modelData.caption
                icon.name: themeIcons.names[modelData.icon]
                Accessible.name: text
                onClicked: page.appWindow.showArea(modelData.index)
            }
        }
    }
    FormCard.FormCard {
        visible: page.areaIndex === 8
        FormCard.FormButtonDelegate {
            objectName: "browseAllActions"
            text: i18n("Expert action catalog")
            description: i18n("All installed technical actions, including diagnostics and maintenance")
            icon.name: themeIcons.names["utilities-terminal"]
            Accessible.name: text
            onClicked: page.appWindow.openCatalog(-1)
        }
    }
    Repeater {
        model: page.areaIndex > 0 ? backend.settings.topics(page.areaIndex) : []
        delegate: FormCard.FormCard {
            required property var modelData
            Layout.fillWidth: true
            FormCard.FormButtonDelegate {
                objectName: "topic_" + modelData.id
                text: modelData.title
                description: modelData.description
                icon.name: themeIcons.names[modelData.icon]
                Accessible.name: text
                onClicked: page.appWindow.openTopic(modelData.id)
            }
        }
    }
    FormCard.FormCard {
        visible: page.areaIndex === 0 || page.areaIndex === 6
        FormCard.FormButtonDelegate {
            objectName: "openWaydroidInstaller"
            text: i18n("Android applications")
            description: i18n("Install and open Waydroid with Google Play or without Google services")
            icon.name: "system-software-install"
            Accessible.name: text
            onClicked: page.appWindow.openWaydroid()
        }
    }
    FormCard.FormCard {
        visible: backend.settings.nativeLinks(page.areaIndex).length > 0
        Repeater {
            model: backend.settings.nativeLinks(page.areaIndex)
            delegate: FormCard.FormButtonDelegate {
                required property var modelData
                text: modelData.title
                description: i18n("Open the original KDE settings application")
                icon.name: themeIcons.names["preferences-system"]
                Accessible.name: text
                enabled: !backend.simulation && !backend.readOnly
                onClicked: backend.settings.openNative(modelData.id)
            }
        }
    }
}
