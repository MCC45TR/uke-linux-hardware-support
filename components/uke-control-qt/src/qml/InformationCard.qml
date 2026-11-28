// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.AbstractCard {
    id: card

    required property var section
    property bool expanded: false
    signal toggleDetails()

    readonly property var rows: section.rows || []
    readonly property var primary: rows.find(row => row.primary)
    readonly property var supportingFacts: rows.filter(row => !row.primary && !row.detail)
    readonly property int previewCount: section.compact ? 1 : 2
    readonly property var details: supportingFacts.slice(previewCount).concat(rows.filter(row => !row.primary && row.detail))
    readonly property string message: {
        if (section.stale)
            return section.stamp ? i18n("Showing the last available reading from %1.", section.stamp)
                                 : i18n("Showing the last available reading.")
        return section.message || ""
    }

    Layout.fillWidth: true
    Layout.minimumWidth: 0
    Layout.alignment: Qt.AlignTop
    padding: Kirigami.Units.largeSpacing + Kirigami.Units.smallSpacing
    showClickFeedback: false
    focusPolicy: Qt.NoFocus
    text: section.title
    Accessible.role: Accessible.Grouping
    Accessible.name: text

    contentItem: ColumnLayout {
        spacing: Kirigami.Units.mediumSpacing

        Kirigami.Heading {
            Layout.fillWidth: true
            level: 4
            text: card.section.title
            textFormat: Text.PlainText
            wrapMode: Text.Wrap
        }

        RowLayout {
            Layout.fillWidth: true
            visible: card.message.length > 0
            spacing: Kirigami.Units.smallSpacing

            Controls.BusyIndicator {
                visible: card.section.loading || false
                running: visible
                Layout.preferredWidth: Kirigami.Units.iconSizes.smallMedium
                Layout.preferredHeight: Layout.preferredWidth
            }
            Kirigami.Icon {
                visible: card.section.stale || card.section.failed || false
                source: "dialog-warning"
                Layout.preferredWidth: Kirigami.Units.iconSizes.smallMedium
                Layout.preferredHeight: Layout.preferredWidth
                Accessible.ignored: true
            }
            Controls.Label {
                Layout.fillWidth: true
                text: card.message
                textFormat: Text.PlainText
                wrapMode: Text.Wrap
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            visible: !!card.primary
            spacing: Kirigami.Units.smallSpacing

            Kirigami.Heading {
                objectName: "primaryValue"
                Layout.fillWidth: true
                level: 1
                font.weight: Font.DemiBold
                text: card.primary ? card.primary.value : ""
                textFormat: Text.PlainText
                wrapMode: Text.Wrap
                Accessible.name: card.primary ? i18nc("measurement label and value for screen readers", "%1, %2", card.primary.title, text) : ""
            }
            Controls.Label {
                objectName: "primaryLabel"
                Layout.fillWidth: true
                text: card.primary ? card.primary.title : ""
                textFormat: Text.PlainText
                wrapMode: Text.Wrap
                Accessible.ignored: true
            }
            Controls.ProgressBar {
                Layout.fillWidth: true
                visible: card.primary && card.primary.progress >= 0 || false
                value: visible ? card.primary.progress : 0
                Accessible.ignored: true // The measured value is already announced by the heading.
            }
        }

        Repeater {
            model: card.supportingFacts.slice(0, card.previewCount)
            delegate: InformationFact {
                required property var modelData
                fact: modelData
            }
        }

        Controls.Label {
            Layout.fillWidth: true
            visible: text.length > 0
            text: card.section.note || ""
            textFormat: Text.PlainText
            wrapMode: Text.Wrap
        }

        Controls.Button {
            objectName: "informationDetails"
            visible: card.details.length > 0
            flat: true
            icon.name: card.expanded ? "go-up" : "go-down"
            text: card.expanded ? i18n("Hide details") : i18n("Show details")
            Accessible.description: card.section.title
            onClicked: card.toggleDetails()
        }

        Loader {
            objectName: "informationDetailRows"
            Layout.fillWidth: true
            active: card.expanded && card.details.length > 0
            sourceComponent: ColumnLayout {
                spacing: Kirigami.Units.mediumSpacing
                Kirigami.Separator { Layout.fillWidth: true }
                Repeater {
                    model: card.details
                    delegate: InformationFact {
                        required property var modelData
                        fact: modelData
                    }
                }
            }
        }
    }
}
