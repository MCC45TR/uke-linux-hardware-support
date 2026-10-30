// Host-only assertions; no Node.js dependency is shipped to the tablet.
const assert = require("node:assert/strict");
const fs = require("node:fs");
const vm = require("node:vm");
const context = vm.createContext({});
vm.runInContext(fs.readFileSync(`${__dirname}/../src/plasma-applet/contents/ui/Control.js`, "utf8"), context);
for (const action of ["plasma", "desktop", "usb", "rotationOn", "rotationOff", "brightnessOn", "brightnessOff", "cdcOn", "cdcOff", "host", "device"])
    assert.match(context.command(action), /^(\/usr\/bin\/pkexec )?\/usr\/bin\/uke-control --json /);
for (const action of ["__proto__", "constructor", "usb; touch /tmp/unsafe", ""])
    assert.equal(context.command(action), null);
for (const action of ["lights", "rgb", "rgbTest", "flashlightOn", "flashlightOff", "flashlightSet"]) {
    assert.equal(context.isLightAction(action), true);
    assert.equal(context.command(action, [50]), null);
}
for (const action of ["__proto__", "host", "ledSet", "flashlightSet;reboot"])
    assert.equal(context.isLightAction(action), false);
for (const action of ["ledSet", "ledOff"])
    assert.equal(context.command(action, [255, 0, 128, 75]), null);
for (const values of [[], [0], [101], [-1], [1.5], ["50"], ["0;reboot"], [0, 1], null])
    assert.equal(context.command("flashlightSet", values), null);
const validRgb = {schema:"org.uke.control.rgb.v1", available:true, mode:"test", test_remaining_ms:10000, kernel_io_error:0};
assert.equal(context.parse("rgb", JSON.stringify(validRgb)).mode, "test");
assert.equal(context.parse("rgb", '{"schema":"org.uke.control.rgb.v1","available":false}').available, false);
for (const [key, value] of [["available", "true"], ["mode", "camera"], ["test_remaining_ms", -1], ["test_remaining_ms", 10001], ["test_remaining_ms", "100"], ["kernel_io_error", 1], ["kernel_io_error", -4096]]) {
    const bad = {...validRgb, [key]: value};
    assert.throws(() => context.parse("rgb", JSON.stringify(bad)));
}
const qml = fs.readFileSync(`${__dirname}/../src/plasma-applet/contents/ui/main.qml`, "utf8");
assert.match(qml, /root\.change\("rgbTest"\)/);
assert.doesNotMatch(qml, /ledSet|ledOff|redValue|greenValue|blueValue/);
assert.equal(context.parse("lights", '{"schema":"org.uke.control.lights.v1","flashlight":{"available":false,"on":null,"brightness_percent":null},"rgb":{"available":false,"on":null,"brightness_percent":null}}').rgb.available, false);
const validLights = {schema:"org.uke.control.lights.v1", flashlight:{available:true,on:true,brightness_percent:50,requested_percent:50}, rgb:{available:true,on:true,brightness_percent:100,color:[255,0,128]}};
assert.equal(context.parse("lights", JSON.stringify(validLights)).rgb.color[2], 128);
for (const p of [0, 101, 1.5, "50", null]) {
    const value = JSON.parse(JSON.stringify(validLights));value.flashlight.requested_percent = p;
    assert.throws(() => context.parse("lights", JSON.stringify(value)));
}
for (const percent of [101, -1, 0.5, "50", null]) {
    const value = JSON.parse(JSON.stringify(validLights));value.flashlight.brightness_percent = percent;
    assert.throws(() => context.parse("lights", JSON.stringify(value)));
}
for (const color of [[256,0,0], [0,0], ["0",0,0], null]) {
    const value = JSON.parse(JSON.stringify(validLights));value.rgb.color = color;
    assert.throws(() => context.parse("lights", JSON.stringify(value)));
}
for (const output of ['{"schema":"org.uke.control.lights.v1"}', '{"schema":"org.uke.control.lights.v1","flashlight":{"available":"true","on":false},"rgb":{"available":false,"on":null}}'])
    assert.throws(() => context.parse("lights", output));
assert.equal(context.parse("plasma", '{"schema":"org.uke.control.plasma.v1","rotation":null,"automatic_brightness":false}').rotation, null);
for (const output of ["null", "invalid", '{"schema":"wrong"}', '{"schema":"org.uke.control.plasma.v1","rotation":"true"}'])
    assert.throws(() => context.parse("plasma", output));
assert.equal(context.property({}, "UPower", "LidIsClosed"), null);
assert.equal(context.property({interfaces: [{service:"UPower", available:false, properties:{LidIsClosed:{value:true}}}]}, "UPower", "LidIsClosed"), null);
console.log("Applet fixed-command boundary, response schemas and unavailable state passed");
const unknown = JSON.parse(JSON.stringify(validLights));
unknown.flashlight.on = null;unknown.flashlight.brightness_percent = null;unknown.flashlight.kernel_io_error = -5;
assert.equal(context.parse("lights", JSON.stringify(unknown)).flashlight.on, null);
unknown.flashlight.kernel_io_error = 0;
assert.throws(() => context.parse("lights", JSON.stringify(unknown)));
assert.match(qml, /import org\.senemos\.uke\.controls/);
assert.match(qml, /lightClient\.request\(action, values/);
assert.match(qml, /else lightClient\.close\(\)/);
