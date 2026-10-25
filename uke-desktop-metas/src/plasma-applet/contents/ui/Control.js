// SPDX-License-Identifier: GPL-3.0-or-later
function command(action, values) {
    if (action === "flashlightSet") {
        const maxima = [100]
        if (!Array.isArray(values) || values.length !== maxima.length || values.some(function(value, index) {
            return !Number.isInteger(value) || value < 0 || value > maxima[index]
        })) return null
        return "/usr/bin/pkexec /usr/bin/uke-control flashlight set " + values.join(" ")
    }
    const commands = {
        plasma: "/usr/bin/uke-control desktop plasma",
        desktop: "/usr/bin/uke-control desktop status",
        usb: "/usr/bin/uke-control usb status",
        lights: "/usr/bin/uke-control flashlight status",
        rgb: "/usr/bin/uke-control led status",
        rgbTest: "/usr/bin/uke-control led test",
        flashlightOn: "/usr/bin/pkexec /usr/bin/uke-control flashlight on",
        flashlightOff: "/usr/bin/pkexec /usr/bin/uke-control flashlight off",
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
        usb: "org.uke.control.usb.v1",
        lights: "org.uke.control.lights.v1",
        rgb: "org.uke.control.rgb.v1"
    }
    if (!value || value.schema !== schemas[kind] || !schemas[kind])
        throw new Error("Unexpected uke-control response")
    if ((kind === "desktop" && !Array.isArray(value.interfaces)) ||
        (kind === "usb" && !Array.isArray(value.typec)) ||
        (kind === "plasma" && [value.rotation, value.automatic_brightness].some(function(item) {
            return item !== null && typeof item !== "boolean"
        })))
        throw new Error("Malformed uke-control state")
    if (kind === "lights") {
        for (const name of ["flashlight", "rgb"]) {
            const light = value[name]
            if (!light || typeof light.available !== "boolean" ||
                (light.available ? typeof light.on !== "boolean" || !Number.isInteger(light.brightness_percent) ||
                    light.brightness_percent < 0 || light.brightness_percent > 100 :
                    light.on !== null || light.brightness_percent !== null))
                throw new Error("Malformed light state")
        }
        if (value.rgb.available && (!Array.isArray(value.rgb.color) || value.rgb.color.length !== 3 ||
            value.rgb.color.some(function(c) { return !Number.isInteger(c) || c < 0 || c > 255 })))
            throw new Error("Malformed RGB color")
    }
    if (kind === "rgb" && (typeof value.available !== "boolean" || (value.available &&
        (["off", "manual", "low-battery", "critical-battery", "front-camera", "test"].indexOf(value.mode) < 0 ||
         !Number.isInteger(value.test_remaining_ms) || value.test_remaining_ms < 0 || value.test_remaining_ms > 10000 ||
         !Number.isInteger(value.kernel_io_error) || value.kernel_io_error > 0 || value.kernel_io_error < -4095))))
        throw new Error("Malformed kernel RGB state")
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
