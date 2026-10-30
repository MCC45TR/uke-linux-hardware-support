// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtTest
import "../src/plasma-applet/contents/ui"

TestCase {
    name: "UkeRequestState"
    Component { id: factory; RequestState {} }
    property var model
    function init() { model = createTemporaryObject(factory, this) }
    function source(action) {
        for (const key of Object.keys(model.requests)) if (model.requests[key].action === action) return key
        fail("No request for " + action)
        return ""
    }
    function finish(action, code, value) {
        model.finish(source(action), {"exit code":code, stdout:JSON.stringify(value)}, false)
    }
    function panel(rotation) { return {schema:"org.uke.control.plasma.v1", rotation:rotation, automatic_brightness:false} }
    function test_independent_domains() {
        model.refresh()
        compare(model.pending, 5)
        verify(model.busy("usb"))
        finish("plasma", 0, panel(true))
        verify(model.execute("rotationOff"))
        verify(!model.execute("rotationOn"))
        verify(!model.execute("host"))
        compare(model.pending, 5)
    }
    function test_read_error_clears_and_old_response_is_ignored() {
        model.execute("plasma")
        const old = source("plasma")
        model.finish(old, {"exit code":1}, false)
        compare(model.availability("plasma", false), "error")
        verify(model.errorText.length > 0)
        model.execute("plasma")
        finish("plasma", 0, panel(true))
        compare(model.errorText, "")
        compare(model.availability("plasma", true), "available")
        model.finish(old, {"exit code":0, stdout:JSON.stringify(panel(false))}, false)
        compare(model.snapshots.plasma.rotation, true)
        compare(model.pending, 0)
    }
    function test_write_error_survives_reads() {
        model.execute("rotationOn")
        finish("rotationOn", 1, {})
        model.execute("plasma")
        finish("plasma", 0, panel(false))
        verify(model.errorText.length > 0)
        compare(model.domains.display.readError, "")
        verify(model.domains.display.writeError.length > 0)
    }
    function test_timeout_preserves_stale_data_and_write_ownership() {
        model.execute("plasma")
        finish("plasma", 0, panel(true))
        model.execute("plasma")
        model.execute("host")
        const old = source("plasma")
        model.expire(Date.now() + 16000)
        compare(model.availability("plasma", true), "stale")
        compare(model.snapshots.plasma.rotation, true)
        verify(model.busy("usb"))
        verify(!model.execute("device"))
        compare(model.pending, 1)
        model.finish(old, {"exit code":0, stdout:JSON.stringify(panel(false))}, false)
        compare(model.snapshots.plasma.rotation, true)
    }
    function test_unknown_unsupported_and_denied() {
        compare(model.availability("plasma", false), "loading")
        model.execute("plasma")
        finish("plasma", 0, panel(null))
        compare(model.availability("plasma", false), "unsupported")
        model.execute("plasma")
        finish("plasma", 77, {})
        compare(model.availability("plasma", false), "denied")
    }
}
