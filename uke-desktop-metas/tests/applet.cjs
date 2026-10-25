// Host-only assertions; no Node.js dependency is shipped to the tablet.
const assert = require("node:assert/strict");
const fs = require("node:fs");
const vm = require("node:vm");
const context = vm.createContext({});
vm.runInContext(fs.readFileSync(`${__dirname}/../src/plasma-applet/contents/ui/Control.js`, "utf8"), context);
for (const action of ["plasma", "desktop", "usb", "rotationOn", "rotationOff", "brightnessOn", "brightnessOff", "cdcOn", "cdcOff", "host", "device"])
    assert.match(context.command(action), /^(\/usr\/bin\/pkexec )?\/usr\/bin\/uke-control /);
for (const action of ["__proto__", "constructor", "usb; touch /tmp/unsafe", ""])
    assert.equal(context.command(action), null);
for (const action of ["lights", "flashlightOn", "flashlightOff", "ledOff"])
    assert.match(context.command(action), /^(\/usr\/bin\/pkexec )?\/usr\/bin\/uke-control /);
assert.equal(context.command("flashlightSet", [50]), "/usr/bin/pkexec /usr/bin/uke-control flashlight set 50");
assert.equal(context.command("ledSet", [255, 0, 128, 75]), "/usr/bin/pkexec /usr/bin/uke-control led set 255 0 128 75");
for (const values of [[], [101], [-1], [1.5], ["50"], ["0;reboot"], [0, 1], null])
    assert.equal(context.command("flashlightSet", values), null);
for (const values of [[256, 0, 0, 50], [0, 0, 0, 101], [0, "0", 0, 50], [0, 0, 0]])
    assert.equal(context.command("ledSet", values), null);
assert.equal(context.parse("lights", '{"schema":"org.uke.control.lights.v1","flashlight":{"available":false,"on":null,"brightness_percent":null},"rgb":{"available":false,"on":null,"brightness_percent":null}}').rgb.available, false);
const validLights = {schema:"org.uke.control.lights.v1", flashlight:{available:true,on:true,brightness_percent:50}, rgb:{available:true,on:true,brightness_percent:100,color:[255,0,128]}};
assert.equal(context.parse("lights", JSON.stringify(validLights)).rgb.color[2], 128);
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
