// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

ColumnLayout {
    id: field

    required property var parameter
    required property string draft
    readonly property var choices: parameter.choices
    readonly property bool stepped: choices.length > 0
    readonly property int selectedIndex: stepped ? choices.findIndex(choice => choice.value === draft) : -1
    signal edited(string value)

    function percent(value) {
        return i18nc("percentage", "%1%", Number(value).toLocaleString(Qt.locale(), 'f', 0))
    }

    Layout.fillWidth: true
    Layout.margins: Kirigami.Units.largeSpacing
    spacing: Kirigami.Units.smallSpacing

    Controls.Label {
        Layout.fillWidth: true
        text: field.stepped && field.selectedIndex >= 0
            ? i18nc("setting and its selected value", "%1 · %2", field.parameter.title, field.choices[field.selectedIndex].title)
            : field.parameter.title
        textFormat: Text.PlainText
        wrapMode: Text.Wrap
    }

    GridLayout {
        Layout.fillWidth: true
        columns: field.stepped ? 1 : 2
        columnSpacing: Kirigami.Units.mediumSpacing
        rowSpacing: 0

        Controls.Slider {
            id: slider
            objectName: "slider_" + field.parameter.key
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            Layout.minimumHeight: Kirigami.Units.gridUnit * 2
            focusPolicy: Qt.StrongFocus
            from: field.stepped ? 0 : field.parameter.minimum
            to: field.stepped ? field.choices.length - 1 : field.parameter.maximum
            stepSize: 1
            snapMode: Controls.Slider.SnapAlways
            value: field.stepped ? Math.max(0, field.selectedIndex) : Number(field.draft)
            Accessible.name: field.parameter.title
            Accessible.description: field.stepped && field.selectedIndex >= 0
                ? field.choices[field.selectedIndex].title : field.parameter.description
            onMoved: field.edited(field.stepped ? field.choices[Math.round(value)].value : String(Math.round(value)))
        }

        Controls.SpinBox {
            objectName: "exact_" + field.parameter.key
            visible: !field.stepped
            from: field.parameter.minimum
            to: field.parameter.maximum
            value: Number(field.draft)
            editable: true
            Accessible.name: field.parameter.title
            Accessible.description: i18n("Enter a percentage from %1 to %2.", from, to)
            onValueModified: field.edited(String(value))
        }

        GridLayout {
            Layout.fillWidth: true
            columns: field.stepped ? field.choices.length : 2
            uniformCellWidths: true
            columnSpacing: Kirigami.Units.smallSpacing

            Repeater {
                model: field.stepped ? field.choices.map(choice => choice.title)
                                     : [field.percent(field.parameter.minimum), field.percent(field.parameter.maximum)]
                delegate: Controls.Label {
                    required property string modelData
                    required property int index
                    Layout.fillWidth: true
                    horizontalAlignment: index === 0 ? Text.AlignLeft
                        : index === (field.stepped ? field.choices.length : 2) - 1 ? Text.AlignRight : Text.AlignHCenter
                    wrapMode: Text.Wrap
                    text: modelData
                    textFormat: Text.PlainText
                    Accessible.ignored: true
                }
            }
        }
    }

    Controls.Label {
        Layout.fillWidth: true
        text: field.parameter.description
        textFormat: Text.PlainText
        wrapMode: Text.Wrap
        visible: text.length > 0
    }
}
