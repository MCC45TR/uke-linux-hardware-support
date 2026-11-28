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
    readonly property real overviewTileHeight: Kirigami.Units.gridUnit * 6
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
                TileColumns {
                    tileHeight: page.overview ? page.overviewTileHeight : -1
                    objectName: (page.overview ? "topicTiles_" : "areaTopicTiles_") + group.modelData.id
                    items: {
                        const topics = backend.settings.topics(group.modelData.index)
                        if (group.modelData.index === 6)
                            return topics.concat({
                                id: "openWaydroidInstaller",
                                title: i18n("Android applications"),
                                description: i18n("Install and open Waydroid with Google Play or without Google services"),
                                icon: "system-software-install"
                            })
                        if (group.modelData.index === 8)
                            return topics.concat({
                                id: "browseAllActions",
                                title: i18n("Expert action catalog"),
                                description: i18n("All installed technical actions, including diagnostics and maintenance"),
                                icon: "utilities-terminal"
                            })
                        return topics
                    }
                    tile: SettingsTile {
                        compactText: page.overview
                        readonly property var topic: tileData
                        objectName: ["openWaydroidInstaller", "browseAllActions"].includes(topic.id)
                                    ? topic.id : "topic_" + topic.id
                        text: topic.title
                        description: topic.description
                        icon.name: themeIcons.names[topic.icon]
                        onClicked: {
                            if (topic.id === "openWaydroidInstaller") page.appWindow.openWaydroid()
                            else if (topic.id === "browseAllActions") page.appWindow.openCatalog(-1)
                            else page.appWindow.openTopic(topic.id)
                        }
                    }
                }
            }
        }

        ColumnLayout {
            visible: page.overview
            Layout.fillWidth: true
            spacing: Kirigami.Units.mediumSpacing

            Kirigami.Heading { level: 3; text: i18n("More") }

            TileColumns {
                tileHeight: page.overviewTileHeight
                items: page.overview ? backend.areas.filter(a => a.secondary) : []
                tile: SettingsTile {
                    compactText: true
                    readonly property var category: tileData
                    objectName: "category_" + category.id
                    text: category.title
                    description: category.caption
                    icon.name: themeIcons.names[category.icon]
                    onClicked: page.appWindow.showArea(category.index)
                }
            }
        }

        ColumnLayout {
            visible: page.nativeLinks.length > 0
            Layout.fillWidth: true
            spacing: Kirigami.Units.mediumSpacing

            Kirigami.Heading { level: 3; text: i18n("KDE System Settings") }

            TileColumns {
                tileHeight: page.overview ? page.overviewTileHeight : -1
                items: page.nativeLinks
                tile: SettingsTile {
                    text: tileData.title
                    icon.name: themeIcons.names["preferences-system"]
                    enabled: !backend.simulation && !backend.readOnly
                    onClicked: backend.settings.openNative(tileData.id)
                }
            }
        }
    }
}
