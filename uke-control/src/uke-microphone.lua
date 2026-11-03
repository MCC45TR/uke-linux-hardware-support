-- SPDX-License-Identifier: MIT
-- Passive, session-owned routing. No recording files, ADC writes or network I/O.
local policy = require("uke-microphone-policy")
local lutils = require("linking-utils")
local log = Log.open_topic("s-uke-microphone")
local modules = {}
local selection, processing, agc = "auto", "calls", false
local nodes = ObjectManager { Interest { type = "node" } }
local function yes(value) return value == true or value == "true" end
local function positions(p)
  if not p["audio.position"] then return nil end
  local ok, value = pcall(function() return Json.Raw(p["audio.position"]):parse() end)
  return ok and value or nil
end
local function qualified(p) return policy.qualified(p, positions(p)) end
local function array_node()
  local found
  for node in nodes:iterate() do
    if qualified(node.properties) then
      if found then return nil end
      found = node
    end
  end
  return found
end
local function source_node(key)
  for node in nodes:iterate() do
    if node.properties["uke.microphone.key"] == key and node.properties["uke.microphone.stage"] == "source" then return node end
  end
end
local function tag(p, key, path, id, stage, engine)
  p["uke.microphone.internal"] = true
  p["uke.microphone.key"] = key
  p["uke.microphone.path"] = path
  p["uke.microphone.id"] = id
  p["uke.microphone.stage"] = stage
  p["uke.microphone.processing"] = engine
  return p
end
local function make_path(array, key, id, speech)
  local serial = array.properties["object.serial"]
  local path = tostring(serial) .. ":" .. key
  local name = speech and ("uke.mic.speech." .. id .. (agc and ".agc-on" or ".agc-off")) or ("uke.mic.selected." .. id)
  local capture = tag({
    ["node.name"] = name .. ".capture", ["target.object"] = serial,
    ["audio.position"] = Json.Array {policy.channels[id]}, ["audio.rate"] = 48000,
    ["stream.dont-remix"] = true, ["node.passive"] = true,
    ["node.dont-fallback"] = true, ["node.dont-reconnect"] = true,
  }, key, path, id, "capture", speech and "webrtc" or "none")
  local source = tag({
    ["node.name"] = name, ["media.class"] = "Audio/Source/Internal",
    ["audio.position"] = Json.Array(speech and {"MONO"} or {"FL","FR"}),
    ["audio.rate"] = 48000, ["node.virtual"] = true, ["node.hidden"] = true,
    ["priority.session"] = 0, ["state.restore-props"] = false,
    ["node.description"] = speech and "Uke konuşma mikrofonu" or "Uke seçili mikrofon",
  }, key, path, id, "source", speech and "webrtc" or "none")
  local library, args
  if speech then
    source["uke.microphone.effects"] = agc and "aec,ns,hpf,agc" or "aec,ns,hpf"
    local reference = tag({
      ["node.name"] = name .. ".reference", ["node.passive"] = true,
      ["stream.capture.sink"] = true, ["stream.monitor"] = true,
      ["audio.position"] = Json.Array {"FL","FR"}, ["audio.rate"] = 48000,
    }, key, path, id, "reference", "webrtc")
    library = "libpipewire-module-echo-cancel"
    args = Json.Object {
      ["library.name"] = "aec/libspa-aec-webrtc", ["monitor.mode"] = true,
      ["audio.position"] = Json.Array {"MONO"}, ["audio.rate"] = 48000,
      ["aec.args"] = Json.Object {
        ["webrtc.high_pass_filter"] = true, ["webrtc.noise_suppression"] = true,
        ["webrtc.gain_control"] = agc,
      },
      ["capture.props"] = Json.Object(capture), ["source.props"] = Json.Object(source),
      ["sink.props"] = Json.Object(reference),
    }
  else
    library = "libpipewire-module-loopback"
    args = Json.Object { ["audio.position"] = Json.Array {"MONO"},
      ["capture.props"] = Json.Object(capture), ["playback.props"] = Json.Object(source) }
  end
  local ok, module = pcall(LocalModule, library, args:get_data(), {})
  if ok and module then
    modules[key] = {module=module, serial=serial, name=name}
  else
    log:warning("Microphone path unavailable; preserving ordinary capture")
  end
end
local function speech_key(id) return "speech:" .. id .. (agc and ":agc-on" or ":agc-off") end
local function reconcile()
  selection = Settings.get_string("uke.microphone.selection") or "auto"
  processing = Settings.get_string("uke.microphone.processing") or "off"
  agc = Settings.get_boolean("uke.microphone.agc") or false
  local array, wanted = array_node(), {}
  if array and policy.channels[selection] then wanted["manual:" .. selection] = {id=selection, speech=false} end
  local speech_id = policy.channels[selection] and selection or (selection == "auto" and "uke-amic1" or nil)
  if array and speech_id and (processing == "calls" or processing == "all") then
    wanted[speech_key(speech_id)] = {id=speech_id, speech=true}
  end
  for key, entry in pairs(modules) do
    local node = source_node(key)
    -- Preferences apply only to new streams. Existing recordings retain their path.
    if not array or entry.serial ~= array.properties["object.serial"] or
        (not wanted[key] and (not node or node:get_state() ~= "running")) then
      entry.module:unload(); modules[key] = nil
    end
  end
  if array then for key, request in pairs(wanted) do
    if not modules[key] then make_path(array, key, request.id, request.speech) end
  end end
end
nodes:connect("object-added", function(_, node)
  if qualified(node.properties) then reconcile() end
  if node.properties["uke.microphone.stage"] == "source" then
    node:connect("state-changed", function() reconcile() end)
  end
end)
nodes:connect("object-removed", function(_, node)
  if qualified(node.properties) then reconcile() end
end)
for _, key in ipairs {"uke.microphone.selection", "uke.microphone.processing", "uke.microphone.agc"} do
  Settings.subscribe(key, reconcile)
end
nodes:activate()
SimpleEventHook {
  name = "linking/uke-microphone-selection",
  after = { "linking/get-filter-from-target" }, before = "linking/prepare-link",
  interests = { EventInterest { Constraint { "event.type", "=", "select-target" } } },
  execute = function(event)
    local _, om, si, props, flags, target = lutils:unwrap_select_target_event(event)
    if not target or flags.was_handled or flags.peer_id or props["media.class"] ~= "Stream/Input/Audio" then return end
    local node, physical = si:get_associated_proxy("node"), target:get_associated_proxy("node")
    if not node or not physical or array_node() ~= physical then return end
    local count = 0
    for port in node:iterate_ports() do if port:get_direction() == "input" then count=count+1 end end
    local p = node.properties
    local _, purpose = policy.context(p)
    local stream = {channels=count, role=p["media.role"], purpose=purpose,
      explicit_target=flags.has_defined_target, internal=yes(p["uke.microphone.internal"]),
      raw=yes(p["uke.microphone.raw"]), dont_filter=yes(p["node.dont-filter"])}
    local chosen = policy.speech(processing, selection, stream, qualified(physical.properties))
    local key = chosen and speech_key(chosen) or nil
    if not key then
      chosen = policy.choose(selection, stream, qualified(physical.properties))
      key = chosen and ("manual:" .. chosen) or nil
    end
    local entry = key and modules[key] or nil
    if not entry or entry.serial ~= physical.properties["object.serial"] then return end
    local replacement = om:lookup {type="SiLinkable", Constraint {"node.name", "=", entry.name}}
    if replacement and lutils.canLink(props, replacement) then event:set_data("target", replacement) end
  end,
}:register()
