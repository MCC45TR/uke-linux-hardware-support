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
    readonly property bool overview: areaIndex === 0
    readonly property var nativeLinks: backend.settings.nativeLinks(areaIndex)

    title: area.title
    actions: [Kirigami.Action {
        text: i18n("Search settings")
        icon.name: themeIcons.names["edit-find"]
        onTriggered: page.appWindow.openSearch()
    }]

    ColumnLayout {
        Layout.fillWidth: true
        Layout.maximumWidth: Kirigami.Units.gridUnit * 68
        Layout.alignment: Qt.AlignHCenter
        Layout.margins: Kirigami.Units.largeSpacing
        spacing: Kirigami.Units.largeSpacing

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            visible: backend.simulation || backend.readOnly
            type: Kirigami.MessageType.Information
            text: backend.simulation ? i18n("Simulation — sample data only. Changes are disabled.") : i18n("Read-only session")
        }

        Repeater {
            model: page.overview ? backend.areas.filter(area => !area.secondary && area.index > 0) : [page.area]
            delegate: ColumnLayout {
                id: group
                required property var modelData
                Layout.fillWidth: true
                spacing: Kirigami.Units.mediumSpacing
                Kirigami.Heading {
                    visible: page.overview && group.modelData.index !== 1
                    Layout.fillWidth: true
                    level: 3
                    text: group.modelData.title
                    wrapMode: Text.Wrap
                }
                TileGrid {
                    objectName: "topicTiles_" + group.modelData.id
                    Repeater {
                        model: backend.settings.topics(group.modelData.index)
                        delegate: SettingsTile {
                            required property var modelData
                            objectName: "topic_" + modelData.id
                            text: modelData.title
                            description: modelData.description
                            icon.name: themeIcons.names[modelData.icon]
                            onClicked: page.appWindow.openTopic(modelData.id)
                        }
                    }
                    SettingsTile {
                        objectName: "openWaydroidInstaller"
                        visible: group.modelData.index === 6
                        text: i18n("Android applications")
                        description: i18n("Install and open Waydroid with Google Play or without Google services")
                        icon.name: themeIcons.names["system-software-install"]
                        onClicked: page.appWindow.openWaydroid()
                    }
                    SettingsTile {
                        objectName: "browseAllActions"
                        visible: group.modelData.index === 8
                        text: i18n("Expert action catalog")
                        description: i18n("All installed technical actions, including diagnostics and maintenance")
                        icon.name: themeIcons.names["utilities-terminal"]
                        onClicked: page.appWindow.openCatalog(-1)
                    }
                }
            }
        }

        ColumnLayout {
            visible: page.overview
            Layout.fillWidth: true
            spacing: Kirigami.Units.mediumSpacing

            Kirigami.Heading { level: 3; text: i18n("More") }

            TileGrid {
                Repeater {
                    model: page.overview ? backend.areas.filter(a => a.secondary) : []
                    delegate: SettingsTile {
                        required property var modelData
                        objectName: "category_" + modelData.id
                        text: modelData.title
                        description: modelData.caption
                        icon.name: themeIcons.names[modelData.icon]
                        onClicked: page.appWindow.showArea(modelData.index)
                    }
                }
            }
        }

        ColumnLayout {
            visible: page.nativeLinks.length > 0
            Layout.fillWidth: true
            spacing: Kirigami.Units.mediumSpacing

            Kirigami.Heading { level: 3; text: i18n("KDE System Settings") }

            TileGrid {
                Repeater {
                    model: page.nativeLinks
                    delegate: SettingsTile {
                        required property var modelData
                        text: modelData.title
                        icon.name: themeIcons.names["preferences-system"]
                        enabled: !backend.simulation && !backend.readOnly
                        onClicked: backend.settings.openNative(modelData.id)
                    }
                }
            }
        }
    }
}
