// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kcmutils as KCM

KCM.SimpleKCM {
    id: page
    implicitWidth: Kirigami.Units.gridUnit * 40
    implicitHeight: Kirigami.Units.gridUnit * 34
    horizontalScrollBarPolicy: QQC2.ScrollBar.AlwaysOff
    function domainTitle(domain) {
        const labels = {battery:i18n("Battery and energy"),charge:i18n("Charging protection"),display:i18n("Display and dock"),usb:i18n("USB"),desktop:i18n("Desktop"),hardware:i18n("Hardware"),sensors:i18n("Sensors and accessories"),audio:i18n("Audio and microphone"),camera:i18n("Camera"),nfc:i18n("NFC"),sleep:i18n("Sleep and wake"),storage:i18n("Storage"),updates:i18n("Updates"),recovery:i18n("Recovery"),workload:i18n("Workload and memory")}
        return labels[domain] || domain
    }
    function stateText(state) {
        const labels = {unknown:i18n("Not read yet"),loading:i18n("Reading…"),available:i18n("Observation available"),unsupported:i18n("Not supported by this provider"),denied:i18n("Permission denied"),stale:i18n("Previous observation is stale"),not_qualified:i18n("Not qualified"),error:i18n("Could not read this component")}
        return labels[state] || i18n("State unknown")
    }
    function metricTitle(id) {
        const labels = {"battery.status":i18n("Reported battery state"),"battery.capacity":i18n("Reported charge"),"battery.voltage_now":i18n("Battery voltage"),"battery.current_now":i18n("Net battery current"),"battery.temp":i18n("Battery temperature"),"battery.cycle_count":i18n("Reported cycles"),"usb.online":i18n("External power reported"),provider_verified:i18n("Provider identity verified"),profile_qualified:i18n("Charging controls qualified"),automatic_admission:i18n("Automatic policy admitted"),rotation:i18n("Automatic rotation"),automatic_brightness:i18n("Automatic brightness"),usb_role:i18n("Reported USB data role"),cdc_policy:i18n("Serial startup policy"),sensor_channels:i18n("Sensor channels found"),alsa_nodes:i18n("ALSA nodes found"),audio_session:i18n("Audio session query succeeded"),video_nodes:i18n("Video nodes found"),media_nodes:i18n("Media nodes found"),nfc_available:i18n("NFC provider available"),nfc_powered:i18n("NFC radio enabled"),sleep_modes:i18n("Kernel sleep modes"),wake_sources:i18n("Wake sources reported"),root_capacity_bytes:i18n("Root filesystem capacity"),root_available_bytes:i18n("Root filesystem available"),selected_channel:i18n("Selected package channel"),package_pending:i18n("Pending package channel"),layout_pending:i18n("Layout maintenance prepared"),resize_pending:i18n("Filesystem growth prepared")}
        return labels[id] || id
    }
    function metricValue(row) {
        if (!row.known) return i18n("Not reported")
        if (row.id === "rotation" && row.value === false) return i18n("Off or tablet mode only")
        if (typeof row.value === "boolean") return row.value ? i18n("Yes") : i18n("No")
        if (row.unit === "bytes" && typeof row.value === "number") return (row.value / 1073741824).toLocaleString(Qt.locale(), "f", 1) + " GiB"
        return String(row.value) + (row.unit ? " " + (row.unit === "cycles" ? i18n("cycles") : row.unit) : "")
    }
    Component.onCompleted: kcm.overview.refresh()
    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing
        RowLayout {
            Layout.fillWidth: true
            Kirigami.Heading { Layout.fillWidth: true; text: i18n("Uke Tablet"); level: 1 }
            QQC2.Button { text: i18n("Refresh"); icon.name: "view-refresh"; Accessible.name: text; onClicked: kcm.overview.refresh() }
        }
        QQC2.Label {
            Layout.fillWidth: true
            visible: kcm.overview.simulation
            text: i18n("Simulation: hardware queries are disabled.")
            wrapMode: Text.WordWrap
        }
        QQC2.Label {
            Layout.fillWidth: true
            text: i18n("Read-only observations. Missing providers do not prevent other cards from loading. Driver reports and physical validation are separate.")
            wrapMode: Text.WordWrap
        }
        Repeater {
            model: kcm.overview.cards
            delegate: QQC2.Frame {
                id: card
                required property var modelData
                Layout.fillWidth: true
                ColumnLayout {
                    width: card.availableWidth
                    RowLayout {
                        Layout.fillWidth: true
                        Kirigami.Heading { Layout.fillWidth: true; text: page.domainTitle(card.modelData.domain); level: 3; wrapMode: Text.WordWrap }
                        QQC2.BusyIndicator { running: card.modelData.state === "loading"; visible: running; implicitWidth: Kirigami.Units.gridUnit * 2; implicitHeight: implicitWidth }
                    }
                    QQC2.Label { Layout.fillWidth: true; text: page.stateText(card.modelData.state); wrapMode: Text.WordWrap; Accessible.name: page.domainTitle(card.modelData.domain) + ": " + text }
                    Repeater {
                        model: card.modelData.metrics || []
                        delegate: ColumnLayout {
                            required property var modelData
                            Layout.fillWidth: true
                            RowLayout {
                                Layout.fillWidth: true
                                QQC2.Label { Layout.fillWidth: true; Layout.minimumWidth: 0; text: page.metricTitle(parent.parent.modelData.id); textFormat: Text.PlainText; wrapMode: Text.WordWrap }
                                QQC2.Label { Layout.minimumWidth: 0; text: page.metricValue(parent.parent.modelData); textFormat: Text.PlainText; wrapMode: Text.WordWrap; Layout.maximumWidth: card.availableWidth / 2 }
                            }
                            QQC2.Label { Layout.fillWidth: true; visible: parent.modelData.id === "battery.current_now"; text: i18n("Positive current flows into the battery."); wrapMode: Text.WordWrap }
                            QQC2.Label {
                                Layout.fillWidth: true
                                visible: !!parent.modelData.validity && parent.modelData.validity !== "reported" && parent.modelData.validity !== "valid"
                                text: i18n("Measurement status: %1", parent.modelData.validity === "unqualified" ? i18n("Not qualified") : String(parent.modelData.validity || "").replace(/_/g, " "))
                                textFormat: Text.PlainText
                                wrapMode: Text.WordWrap
                            }
                        }
                    }
                    QQC2.Label { Layout.fillWidth: true; visible: !!card.modelData.observed; text: i18n("Last observation: %1", card.modelData.observed || ""); wrapMode: Text.WordWrap }
                    QQC2.Button { id: details; text: checked ? i18n("Hide details") : i18n("Show details"); checkable: true; visible: !!card.modelData.details; Accessible.name: page.domainTitle(card.modelData.domain) + ": " + text }
                    QQC2.TextArea {
                        Layout.fillWidth: true
                        visible: details.checked
                        readOnly: true
                        selectByMouse: true
                        wrapMode: TextEdit.WrapAnywhere
                        textFormat: TextEdit.PlainText
                        text: card.modelData.details || ""
                        Accessible.name: i18n("Component observations")
                    }
                }
            }
        }
    }
}
