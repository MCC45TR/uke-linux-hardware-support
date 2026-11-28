import QtQuick
import QtTest
import "../../src/qml" as Uke

TestCase {
    id: test
    name: "WetTouchControl"
    visible: true
    width: 1180
    height: 860
    when: windowShown

    function i18n(value) { return value }
    function i18nc(context, value, arg) { return value.replace("%1", arg) }

    property QtObject backend: QtObject {
        property bool readOnly: false
        property bool simulation: false
    }

    QtObject {
        id: fake
        property var desktopDisplay: status(false, false, false)
        property var displayResources: ({read: false, available: false})
        property var topic: ({changes: []})
        property var requests: []

        function status(supported, active, pending) {
            return {
                manualBrightness: -1, available: false, ambientP3: false, overlayAvailable: false,
                liveVrr: false, dscSharing: false, message: "", busy: pending,
                wetTouchAvailable: supported, wetTouchEnabled: active
            }
        }

        function nativeLinks() { return [] }
        function setDisplayPreference(id, value) {
            requests.push([id, value])
            desktopDisplay = status(true, desktopDisplay.wetTouchEnabled, true)
        }
    }

    Uke.DisplayPreferencesPanel {
        id: panel
        anchors.fill: parent
        settings: fake
        appWindow: test
    }

    function test_wet_setting_data() {
        return [{tag: "wide", width: 1180}, {tag: "narrow", width: 520}]
    }

    function test_wet_setting(data) {
        test.width = data.width
        fake.requests = []
        test.backend.readOnly = false
        fake.desktopDisplay = fake.status(false, false, false)
        const item = findChild(panel, "wetTouchFilter")
        verify(item)
        compare(item.visible, false)
        fake.desktopDisplay = fake.status(true, false, false)
        tryCompare(item, "visible", true)
        compare(item.checked, false)
        mouseClick(item)
        tryCompare(item, "enabled", false)
        compare(fake.requests.length, 1)
        compare(fake.requests[0], ["touch.wet-filter", "1"])
        fake.desktopDisplay = fake.status(true, true, false)
        tryCompare(item, "checked", true)
        tryCompare(item, "enabled", true)
        mouseClick(findChild(item, "checkControl"))
        tryCompare(item, "enabled", false)
        compare(fake.requests.length, 2)
        compare(fake.requests[1], ["touch.wet-filter", "0"])
        fake.desktopDisplay = fake.status(true, true, false)
        tryCompare(item, "checked", true)
        item.forceActiveFocus(Qt.TabFocusReason)
        tryCompare(item, "activeFocus", true)
        keyClick(Qt.Key_Space)
        tryCompare(item, "enabled", false)
        compare(fake.requests.length, 3)
        compare(fake.requests[2], ["touch.wet-filter", "0"])
        fake.desktopDisplay = fake.status(true, false, false)
        tryCompare(item, "checked", false)
        test.backend.readOnly = true
        tryCompare(item, "enabled", false)
    }
}
