-- SPDX-License-Identifier: MIT
local p = dofile(arg[1])
local card = { ["media.class"] = "Audio/Source", ["device.api"] = "alsa",
  ["alsa.card_name"] = "POCO-Pad-X1", ["alsa.components"] = "mic:uke-4ch-v4 kernel:7.3.1-106",
  ["audio.channels"] = "4" }
local positions = { "AUX0", "AUX1", "AUX2", "AUX3" }
assert(p.qualified(card, positions))
for _, marker in ipairs { "mic:uke-4ch-v3", "mic:uke-4ch-v4", "mic:uke-4ch-v4 kernel:evil" } do
  card["alsa.components"] = marker
  assert(not p.qualified(card, positions))
end
card["alsa.components"] = "mic:uke-4ch-v4 kernel:7.3.1-106"
assert(not p.qualified(card, { "AUX1", "AUX0", "AUX2", "AUX3" }))
for id, channel in pairs(p.channels) do
  assert(channel:match("^AUX[0-3]$"))
  assert(p.choose(id, {channels=1}, true) == id)
  assert(p.choose(id, {channels=2}, true) == id)
  assert(not p.choose(id, {channels=4}, true))
  assert(not p.choose(id, {channels=2, explicit_target=true}, true))
  assert(not p.choose(id, {channels=2, raw=true}, true))
  assert(not p.choose(id, {channels=2, role="Production"}, true))
  assert(not p.choose(id, {channels=2, internal=true}, true))
  assert(not p.choose(id, {channels=2}, false))
end
assert(not p.choose("auto", {channels=1}, true))
assert(not p.choose("unknown", {channels=1}, true))
assert(not p.choose("uke-amic1", {}, true))
local camera, purpose = p.context { ["uke.microphone.camera"]="rear", ["uke.microphone.purpose"]="scene" }
assert(camera == "rear" and purpose == "scene")
camera, purpose = p.context { ["application.name"]="Chrome" }
assert(camera == "unknown" and purpose == "unknown")
camera, purpose = p.context { ["media.role"]="Communication" }
assert(camera == "unknown" and purpose == "call")
print("Microphone identity, manual precedence, raw/external fallback and context checks passed")
