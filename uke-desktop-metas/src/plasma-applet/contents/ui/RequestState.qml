// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import "Control.js" as Control

QtObject {
    id: state
    property var snapshots: ({})
    property var domains: ({})
    property var requests: ({})
    property int serial: 0
    property int pending: 0
    property string readFailureText: "Could not read tablet state. Refresh to try again."
    property string writeFailureText: "The change failed or authorization was cancelled."
    property string timeoutText: "The state query timed out. Refresh to try again."
    readonly property string errorText: {
        const messages = []
        for (const name of Object.keys(domains)) {
            const entry = domains[name]
            if (entry.readError) messages.push(entry.readError)
            if (entry.writeError) messages.push(entry.writeError)
        }
        return [...new Set(messages)].join("\n")
    }
    signal dispatch(string source)
    signal release(string source)
    signal refreshRequested()

    function busy(domain) { return domains[domain]?.phase === "loading" || domains[domain]?.phase === "applying" }
    function availability(action, supported) {
        const entry = domains[Control.domain(action)]
        if (!entry || (!entry.readAt && !entry.readError)) return "loading"
        if (entry.readError) return entry.denied ? "denied" : snapshots[action] ? "stale" : "error"
        return supported ? "available" : "unsupported"
    }
    function execute(action, values) {
        const command = Control.command(action, values)
        const domain = Control.domain(action)
        if (!command || !domain || busy(domain)) return false
        const source = "UKE_TRAY_REQUEST=" + (++serial) + " " + command
        const entry = Object.assign({}, domains[domain] || {})
        const query = Control.isQuery(action)
        entry.phase = query ? "loading" : "applying"
        entry.request = serial
        if (!query) entry.writeError = ""
        domains = Object.assign({}, domains, {[domain]: entry})
        requests = Object.assign({}, requests, {[source]: {action:action, domain:domain, query:query, request:serial, started:Date.now()}})
        ++pending
        dispatch(source)
        return true
    }
    function refresh() {
        for (const action of ["plasma", "desktop", "usb", "lights", "rgb"]) execute(action)
    }
    function finish(source, data, timedOut) {
        const request = requests[source]
        if (!request) return
        const remaining = Object.assign({}, requests)
        delete remaining[source]
        requests = remaining
        --pending
        release(source)
        const entry = Object.assign({}, domains[request.domain])
        if (entry.request !== request.request) return
        entry.phase = "idle"
        if (request.query) {
            try {
                if (timedOut || data["exit code"] !== 0) throw new Error("read failed")
                const value = Control.parse(request.action, String(data.stdout || ""))
                snapshots = Object.assign({}, snapshots, {[request.action]: value})
                entry.readError = ""
                entry.denied = false
                entry.readAt = Date.now()
            } catch (error) {
                entry.readError = timedOut ? timeoutText : readFailureText
                entry.denied = data["exit code"] === 77 || String(data.stderr || "").indexOf("org.freedesktop.DBus.Error.AccessDenied") >= 0
            }
        } else {
            entry.writeError = data["exit code"] === 0 ? "" : writeFailureText
        }
        domains = Object.assign({}, domains, {[request.domain]: entry})
        if (!request.query) refreshRequested()
    }
    function expire(now) {
        for (const source of Object.keys(requests)) {
            const request = requests[source]
            // A slow authorized write stays owned until its actual result arrives.
            if (request.query && now - request.started >= 15000) finish(source, {}, true)
        }
    }
}
