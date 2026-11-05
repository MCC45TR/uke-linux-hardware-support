// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard
ColumnLayout {
    id: view
    required property var manager
    required property int seconds
    property string group: "thermal"
    property string search: ""
    property string selectedId: ""
    property int shown: 12
    readonly property var catalog: manager.sensors.filter(s => (group === "all" || s.group === group) && (s.title + " " + s.id).toLowerCase().includes(search.toLowerCase()))
    readonly property var descriptor: catalog.find(s => s.id === selectedId) || ({})
    function choose() { if (!catalog.some(s => s.id === selectedId)) selectedId = catalog.length ? catalog[0].id : "" }
    onCatalogChanged: Qt.callLater(choose)
    onSelectedIdChanged: if (selectedId.length) manager.selectSensor(selectedId)
    Layout.fillWidth: true
    spacing: 0
    FormCard.FormCard {
        Layout.fillWidth: true
        SafeText { text: i18n("Sensor observations"); description: i18n("Kernel thermal, hwmon and IIO readings are shown with their source units. Desktop light and motion values may be cached; source sample age can be unknown. A missing reading is never replaced with zero.") }
        FormCard.FormComboBoxDelegate {
            text: i18n("Sensor group")
            model: [{title:i18n("Temperature zones"),value:"thermal"},{title:i18n("Analog and IIO sensors"),value:"iio"},{title:i18n("Hardware monitors"),value:"hwmon"},{title:i18n("Desktop light and motion"),value:"desktop"},{title:i18n("All sensors"),value:"all"}]
            textRole: "title"
            currentIndex: { for (let i=0;i<model.length;++i) if (model[i].value === view.group) return i; return -1 }
            onActivated: index => { view.group = model[index].value; view.shown = 12 }
        }
        FormCard.FormTextFieldDelegate { label: i18n("Find a sensor"); text: view.search; onTextEdited: { view.search = text; view.shown = 12 } }
        FormCard.FormComboBoxDelegate {
            objectName: "sensorSelector"
            text: i18n("Sensor to graph")
            model: view.catalog
            textRole: "title"
            currentIndex: { for (let i=0;i<model.length;++i) if (model[i].id === view.selectedId) return i; return -1 }
            onActivated: index => view.selectedId = model[index].id
        }
        SafeText { text: i18n("Selected source"); description: view.selectedId }
    }
    MetricChart { manager: view.manager; metric: "sensor:" + view.selectedId; heading: view.descriptor.title || i18n("Sensor history"); unit: view.descriptor.unit || ""; seconds: view.seconds }
    FormCard.FormHeader { title: i18n("Readings · %1", view.catalog.length) }
    Repeater {
        model: view.catalog.slice(0, view.shown)
        delegate: FormCard.FormCard {
            required property var modelData
            readonly property var reading: view.manager.state.sensorValues ? (view.manager.state.sensorValues[modelData.id] || {}) : ({})
            Layout.fillWidth: true
            SafeText { text: modelData.title; description: reading.value !== null && reading.value !== undefined ? view.manager.format(reading.value, modelData.unit) : reading.text || i18n("Not reported") }
            SafeText { text: i18n("Source state"); description: reading.status || i18n("Not reported") }
        }
    }
    FormCard.FormCard {
        visible: view.catalog.length > view.shown
        Layout.fillWidth: true
        FormCard.FormButtonDelegate { text: i18n("Show more sensors"); icon.name: themeIcons.names["view-list-details"]; onClicked: view.shown += 12 }
    }
}
