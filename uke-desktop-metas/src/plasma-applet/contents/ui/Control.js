// SPDX-License-Identifier: GPL-3.0-or-later
function isLightAction(action) {
    return ["lights", "rgb", "rgbTest", "rgbSet", "rgbAuto", "flashlightOn", "flashlightOff", "flashlightSet"].indexOf(action) >= 0
}
function command(action, values) {
    const commands = {
        plasma: "/usr/bin/uke-control --json desktop plasma",
        desktop: "/usr/bin/uke-control --json desktop status",
        usb: "/usr/bin/uke-control --json usb status",
        rotationOn: "/usr/bin/uke-control --json desktop rotation on",
        rotationOff: "/usr/bin/uke-control --json desktop rotation off",
        brightnessOn: "/usr/bin/uke-control --json desktop brightness on",
        brightnessOff: "/usr/bin/uke-control --json desktop brightness off",
        cdcOn: "/usr/bin/pkexec /usr/bin/uke-control --json cdc on --persistent",
        cdcOff: "/usr/bin/pkexec /usr/bin/uke-control --json cdc off --persistent",
        host: "/usr/bin/pkexec /usr/bin/uke-control --json usb role host",
        device: "/usr/bin/pkexec /usr/bin/uke-control --json usb role device"
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
            const unknown = light && light.available === true && light.on === null && light.brightness_percent === null &&
                Number.isInteger(light.kernel_io_error) && light.kernel_io_error < 0 && light.kernel_io_error >= -4095
            if (!light || typeof light.available !== "boolean" ||
                (light.available ? !unknown && (typeof light.on !== "boolean" || !Number.isInteger(light.brightness_percent) ||
                    light.brightness_percent < 0 || light.brightness_percent > 100) :
                    light.on !== null || light.brightness_percent !== null))
                throw new Error("Malformed light state")
        }
        if (value.flashlight.available && (!Number.isInteger(value.flashlight.requested_percent) ||
            value.flashlight.requested_percent < 1 || value.flashlight.requested_percent > 100))
            throw new Error("Malformed torch request state")
        if (value.rgb.available && (!Array.isArray(value.rgb.color) || value.rgb.color.length !== 3 ||
            value.rgb.color.some(function(c) { return !Number.isInteger(c) || c < 0 || c > 255 })))
            throw new Error("Malformed RGB color")
    }
    if (kind === "rgb" && (typeof value.available !== "boolean" || (value.available &&
        (["off", "manual", "low-battery", "critical-battery", "front-camera", "test"].indexOf(value.mode) < 0 ||
         !Number.isInteger(value.test_remaining_ms) || value.test_remaining_ms < 0 || value.test_remaining_ms > 10000 ||
         !Number.isInteger(value.kernel_io_error) || value.kernel_io_error > 0 || value.kernel_io_error < -4095))))
        throw new Error("Malformed kernel RGB state")
    if (kind === "rgb" && value.available) {
        for (const key of ["color_rgb", "requested_color_rgb"])
            if (!Array.isArray(value[key]) || value[key].length !== 3 || value[key].some(function(c) {
                return !Number.isInteger(c) || c < 0 || c > 255
            })) throw new Error("Malformed RGB channels")
        if (!Number.isInteger(value.requested_percent) || value.requested_percent < 0 || value.requested_percent > 100)
            throw new Error("Malformed RGB brightness request")
    }
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
