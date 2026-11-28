// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

ColumnLayout {
    id: panel
    required property var settings
    required property var information
    property bool currentPage: false
    readonly property var charger: settings.chargeTelemetry.state
    onCurrentPageChanged: settings.setChargingVisible(currentPage)
    Component.onCompleted: settings.setChargingVisible(currentPage)
    Component.onDestruction: if (currentPage) settings.setChargingVisible(false)
    readonly property var snapshot: settings.chargingThresholds
    readonly property var bands: settings.chargingBands
    readonly property var savedBand: ({
            mode: "",
            start: snapshot.start,
            stop: snapshot.stop
        })
    // Keep a saved pair outside the current catalog as a position without a request.
    readonly property var choices: {
        if (!snapshot.available || bands.some(band => band.start === snapshot.start && band.stop === snapshot.stop))
            return bands;
        return [savedBand].concat(bands).sort((a, b) => a.start - b.start || a.stop - b.stop);
    }
    readonly property int savedIndex: choices.findIndex(band => band.start === snapshot.start && band.stop === snapshot.stop)
    readonly property int selectedIndex: selectedMode ? choices.findIndex(band => band.mode === selectedMode) : savedIndex
    readonly property bool canEdit: snapshot.admitted && !backend.readOnly && choices.length > 1
    readonly property var displayed: canEdit && selectedIndex >= 0 ? choices[selectedIndex] : savedBand
    readonly property bool hasChange: canEdit && displayed.mode.length > 0 && selectedIndex !== savedIndex
    property string selectedMode: ""

    onCanEditChanged: {
        if (!canEdit)
            selectedMode = "";
    }

    spacing: Kirigami.Units.largeSpacing

    Kirigami.AbstractCard {
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        padding: Kirigami.Units.largeSpacing + Kirigami.Units.smallSpacing
        showClickFeedback: false
        focusPolicy: Qt.NoFocus
        Accessible.role: Accessible.Grouping
        Accessible.name: i18n("Charging thresholds")

        contentItem: ColumnLayout {
            spacing: Kirigami.Units.mediumSpacing

            Kirigami.Heading {
                Layout.fillWidth: true
                level: 3
                text: i18n("Charging thresholds")
                wrapMode: Text.Wrap
            }

            TileGrid {
                minimumTileWidth: Kirigami.Units.gridUnit * 22
                Repeater {
                    model: [
                        {
                            key: "start",
                            name: i18n("Resume charging"),
                            description: i18n("Charging can resume as the battery drops to this level.")
                        },
                        {
                            key: "stop",
                            name: i18n("Pause charging"),
                            description: i18n("Charging can pause when the battery reaches this level.")
                        }
                    ]

                    delegate: ColumnLayout {
                        id: field
                        required property var modelData
                        readonly property int percent: panel.displayed[modelData.key]
                        readonly property string percentLabel: panel.snapshot.available ? i18nc("percentage", "%1%", percent) : i18n("Not reported")
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignTop
                        spacing: Kirigami.Units.smallSpacing

                        RowLayout {
                            Layout.fillWidth: true
                            Controls.Label {
                                Layout.fillWidth: true
                                text: field.modelData.name
                                font.weight: Font.DemiBold
                                wrapMode: Text.Wrap
                            }
                            Controls.Label {
                                objectName: "chargePercent_" + field.modelData.key
                                Layout.alignment: Qt.AlignTop
                                font.weight: Font.DemiBold
                                text: field.percentLabel
                            }
                        }
                        Controls.Slider {
                            objectName: "chargeSlider_" + field.modelData.key
                            Layout.fillWidth: true
                            Layout.minimumWidth: 0
                            Layout.minimumHeight: Kirigami.Units.gridUnit * 2
                            from: 0
                            to: panel.canEdit ? panel.choices.length - 1 : 100
                            stepSize: 1
                            snapMode: Controls.Slider.SnapAlways
                            value: panel.canEdit ? Math.max(0, panel.selectedIndex) : Math.max(0, field.percent)
                            enabled: panel.canEdit
                            opacity: panel.snapshot.available ? 1 : 0
                            Accessible.name: i18nc("charging threshold name and percentage", "%1, %2", field.modelData.name, field.percentLabel)
                            Accessible.description: field.modelData.description
                            onMoved: {
                                const index = Math.round(value);
                                if (panel.canEdit && panel.choices[index])
                                    panel.selectedMode = index === panel.savedIndex ? "" : panel.choices[index].mode;
                            }
                        }
                        Controls.Label {
                            Layout.fillWidth: true
                            text: field.modelData.description
                            wrapMode: Text.Wrap
                        }
                    }
                }
            }

            Kirigami.Separator {
                Layout.fillWidth: true
            }
            Controls.Label {
                Layout.fillWidth: true
                visible: text.length > 0
                text: {
                    const state = panel.snapshot;
                    if (state.stale)
                        return state.stamp ? i18n("Showing the last available reading from %1.", state.stamp) : i18n("Showing the last available reading.");
                    if (panel.settings.busy)
                        return i18n("Reading…");
                    if (panel.canEdit)
                        return i18n("The available charging bands move both thresholds together.");
                    if (!state.available)
                        return i18n("The device did not report saved charging thresholds.");
                    if (backend.readOnly)
                        return i18n("Read-only session");
                    if (state.profileUnqualified)
                        return i18n("The charging profile for this kernel only allows viewing saved thresholds.");
                    if (!state.admitted)
                        return i18n("This device currently allows you to view saved thresholds only.");
                    return i18n("There are no other supported threshold pairs.");
                }
                wrapMode: Text.Wrap
            }
            Controls.Label {
                Layout.fillWidth: true
                visible: panel.snapshot.available && typeof panel.snapshot.enabled === "boolean"
                text: panel.snapshot.physicalStopVerified ? i18n("Charging stopped at the threshold.")
                    : panel.snapshot.enabled ? i18n("The firmware reports thresholds enabled. Physical cutoff has yet to be verified.")
                    : i18n("The firmware reports thresholds disabled.")
                font: Kirigami.Theme.smallFont
                wrapMode: Text.Wrap
            }

            Controls.Button {
                objectName: "reviewChargingThresholds"
                Layout.alignment: Qt.AlignRight
                visible: panel.canEdit
                enabled: panel.hasChange
                text: i18n("Review thresholds")
                onClicked: {
                    if (panel.hasChange && backend.openAction("charge.mode")) {
                        backend.setDraft("mode", panel.displayed.mode);
                        backend.submit();
                    }
                }
            }
        }
    }

    InformationGrid {
        Layout.maximumWidth: Number.POSITIVE_INFINITY
        Layout.margins: 0
        sections: [{
            key: "live-adapter",
            title: i18n("Battery and adapter telemetry"),
            previewCount: 5,
            message: panel.charger.message || "",
            rows: panel.charger.rows || [],
            note: [
                panel.settings.supportedChargeProtocols.length
                    ? i18n("Protocols listed by the manufacturer: %1", panel.settings.supportedChargeProtocols.join(", "))
                    : "",
                panel.charger.wired && panel.settings.chargeMiFc2Unavailable
                    ? i18n("This Uke version lacks the authentication support required for MI FC2.0 turbo charging.")
                    : ""
            ].filter(Boolean).join("\n")
        }]
    }

    InformationGrid {
        objectName: "informationTiles"
        Layout.maximumWidth: Number.POSITIVE_INFINITY
        Layout.margins: 0
        sections: panel.information
            .filter(section => section.id.startsWith("battery.") && section.id !== "battery.protocol")
            .map(section => section.id === "battery.status"
                ? Object.assign({}, section, {rows: section.rows.filter(row => row.key !== "battery-state")})
                : section)
    }
    BatteryHistoryView {}
    InformationGrid {
        Layout.maximumWidth: Number.POSITIVE_INFINITY
        Layout.margins: 0
        sections: panel.information.filter(section =>
            ["overview.sleep", "wake.status"].includes(section.id))
    }
}
