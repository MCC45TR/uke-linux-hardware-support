// SPDX-License-Identifier: GPL-3.0-or-later
function command(action) {
    const commands = {
        plasma: "/usr/bin/uke-control desktop plasma",
        desktop: "/usr/bin/uke-control desktop status",
        usb: "/usr/bin/uke-control usb status",
        rotationOn: "/usr/bin/uke-control desktop rotation on",
        rotationOff: "/usr/bin/uke-control desktop rotation off",
        brightnessOn: "/usr/bin/uke-control desktop brightness on",
        brightnessOff: "/usr/bin/uke-control desktop brightness off",
        cdcOn: "/usr/bin/pkexec /usr/bin/uke-control set cdc on --persistent",
        cdcOff: "/usr/bin/pkexec /usr/bin/uke-control set cdc off --persistent",
        host: "/usr/bin/pkexec /usr/bin/uke-control usb request data host",
        device: "/usr/bin/pkexec /usr/bin/uke-control usb request data device"
    }
    return Object.prototype.hasOwnProperty.call(commands, action) ? commands[action] : null
}

function parse(kind, output) {
    const value = JSON.parse(output)
    const schemas = {
        plasma: "org.uke.control.plasma.v1",
        desktop: "org.uke.control.desktop.v1",
        usb: "org.uke.control.usb.v1"
    }
    if (!value || value.schema !== schemas[kind] || !schemas[kind])
        throw new Error("Unexpected uke-control response")
    if ((kind === "desktop" && !Array.isArray(value.interfaces)) ||
        (kind === "usb" && !Array.isArray(value.typec)) ||
        (kind === "plasma" && [value.rotation, value.automatic_brightness].some(function(item) {
            return item !== null && typeof item !== "boolean"
        })))
        throw new Error("Malformed uke-control state")
    return value
}

function property(snapshot, service, name) {
    const interfaces = snapshot.interfaces || []
    for (let i = 0; i < interfaces.length; ++i) {
        const item = interfaces[i]
        if (item.service === service && item.available === true && item.properties && item.properties[name])
            return item.properties[name].value
    }
    return null
}
