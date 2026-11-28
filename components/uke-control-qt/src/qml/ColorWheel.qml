// SPDX-License-Identifier: MIT
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

ColumnLayout {
    id: spectrum
    property real hue: 0.58
    property real saturation: 0.85
    property real value: 1
    readonly property color color: Qt.hsva(hue, saturation, value, 1)

    Canvas {
        id: face
        Layout.alignment: Qt.AlignHCenter
        Layout.preferredWidth: Math.min(spectrum.width, Kirigami.Units.gridUnit * 13)
        Layout.preferredHeight: Layout.preferredWidth
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
        onPaint: {
            const ctx = getContext("2d")
            ctx.reset()
            ctx.clearRect(0, 0, width, height)
            const radius = width / 2
            if (radius <= 1) return
            for (let degree = 0; degree < 360; degree += 2) {
                ctx.fillStyle = Qt.hsva(degree / 360, 1, 1, 1)
                ctx.beginPath()
                ctx.moveTo(radius, radius)
                ctx.arc(radius, radius, radius - 1, degree * Math.PI / 180, (degree + 2.4) * Math.PI / 180)
                ctx.closePath()
                ctx.fill()
            }
            const saturation = ctx.createRadialGradient(radius, radius, 0, radius, radius, radius - 1)
            saturation.addColorStop(0, "white")
            saturation.addColorStop(1, "transparent")
            ctx.fillStyle = saturation
            ctx.beginPath()
            ctx.arc(radius, radius, radius - 1, 0, 2 * Math.PI)
            ctx.fill()
        }

        Rectangle {
            width: Kirigami.Units.gridUnit
            height: width
            radius: width / 2
            color: spectrum.color
            border.color: Kirigami.Theme.highlightedTextColor
            border.width: 2
            x: face.width / 2 + Math.cos(spectrum.hue * 2 * Math.PI) * (face.width - width) / 2 * spectrum.saturation - width / 2
            y: face.height / 2 + Math.sin(spectrum.hue * 2 * Math.PI) * (face.height - height) / 2 * spectrum.saturation - height / 2
        }

        MouseArea {
            anchors.fill: parent
            function pick(x, y) {
                const dx = x - width / 2
                const dy = y - height / 2
                const angle = Math.atan2(dy, dx) / (2 * Math.PI)
                spectrum.hue = (angle + 1) % 1
                spectrum.saturation = Math.min(1, 2 * Math.hypot(dx, dy) / width)
                spectrum.value = 1
            }
            onPressed: point => pick(point.x, point.y)
            onPositionChanged: point => { if (pressed) pick(point.x, point.y) }
        }
    }

    Controls.TextField {
        Layout.fillWidth: true
        Accessible.name: i18n("RGB hexadecimal colour")
        text: spectrum.color.toString().toUpperCase()
        maximumLength: 7
        validator: RegularExpressionValidator { regularExpression: /^#[0-9A-Fa-f]{6}$/ }
        onAccepted: {
            if (!acceptableInput) return
            const color = Qt.color(text)
            spectrum.hue = Math.max(0, color.hsvHue)
            spectrum.saturation = color.hsvSaturation
            spectrum.value = color.hsvValue
            text = Qt.binding(() => spectrum.color.toString().toUpperCase())
        }
    }
}
