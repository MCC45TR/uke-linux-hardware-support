-- SPDX-License-Identifier: MIT
-- Event-only manual routing. The raw ALSA node and its power lifecycle are unchanged.
local policy = require("uke-microphone-policy")
local lutils = require("linking-utils")
local log = Log.open_topic("s-uke-microphone")
local modules = {}
local selection = "auto"
local nodes = ObjectManager { Interest { type = "node" } }
local links = ObjectManager { Interest { type = "link" } }

local function positions(p)
  if not p["audio.position"] then return nil end
  local ok, value = pcall(function() return Json.Raw(p["audio.position"]):parse() end)
  return ok and value or nil
end
local function qualified(p)
  return policy.qualified(p, positions(p))
end
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
local function internal_node(id)
  for node in nodes:iterate() do
    if node.properties["uke.microphone.internal"] == "true" and node.properties["uke.microphone.id"] == id then
      return node
    end
  end
end
local function in_use(node)
  if not node then return false end
  if node:get_state() == "running" then return true end
  for link in links:iterate() do
    if tonumber(link.properties["link.output.node"]) == node["bound-id"] then return true end
  end
  return false
end
local function reconcile()
  selection = Settings.get_string("uke.microphone.selection") or "auto"
  local array = array_node()
  for id, entry in pairs(modules) do
    local node = internal_node(id)
    -- Never tear down a path used by an existing recording.
    if not array or ((id ~= selection or entry.serial ~= array.properties["object.serial"])
        and not in_use(node)) then
      entry.module:unload()
      modules[id] = nil
    end
  end
  if not array or not policy.channels[selection] or modules[selection] then return end
  local serial = array.properties["object.serial"]
  local capture = {
    ["node.name"] = "uke.mic.capture." .. selection,
    ["target.object"] = serial,
    ["audio.position"] = Json.Array { policy.channels[selection] },
    ["audio.rate"] = 48000,
    ["stream.dont-remix"] = true,
    ["node.passive"] = true,
    ["node.dont-fallback"] = true,
    ["node.dont-reconnect"] = true,
    ["uke.microphone.internal"] = true,
  }
  local playback = {
    ["node.name"] = "uke.mic.selected." .. selection,
    ["media.class"] = "Audio/Source/Internal",
    ["audio.position"] = Json.Array { "FL", "FR" },
    ["audio.rate"] = 48000,
    ["node.description"] = "Uke selected microphone channel",
    ["node.virtual"] = true,
    ["node.hidden"] = true,
    ["priority.session"] = 0,
    ["state.restore-props"] = false,
    ["uke.microphone.internal"] = true,
    ["uke.microphone.id"] = selection,
  }
  local args = Json.Object {
    ["audio.position"] = Json.Array { "MONO" },
    ["capture.props"] = Json.Object(capture),
    ["playback.props"] = Json.Object(playback),
  }
  local ok, module = pcall(LocalModule, "libpipewire-module-loopback", args:get_data(), {})
  if ok and module then modules[selection] = { module = module, serial = serial }
  else log:warning("Manual microphone path unavailable; preserving raw capture") end
end

nodes:connect("object-added", function(_, node)
  if qualified(node.properties) then reconcile() end
  if node.properties["uke.microphone.id"] then
    node:connect("state-changed", function() reconcile() end)
  end
end)
nodes:connect("object-removed", function(_, node)
  if qualified(node.properties) then reconcile() end
end)
Settings.subscribe("uke.microphone.selection", reconcile)
links:connect("object-removed", function() Core.sync(reconcile) end)
links:activate()
nodes:activate()

SimpleEventHook {
  name = "linking/uke-microphone-selection",
  after = { "linking/get-filter-from-target" },
  before = "linking/prepare-link",
  interests = { EventInterest { Constraint { "event.type", "=", "select-target" } } },
  execute = function(event)
    local _, om, si, props, flags, target = lutils:unwrap_select_target_event(event)
    if not target or flags.was_handled or flags.peer_id or props["media.class"] ~= "Stream/Input/Audio" then return end
    local node = si:get_associated_proxy("node")
    local physical = target:get_associated_proxy("node")
    if not node or not physical then return end
    local channels = 0
    for port in node:iterate_ports() do
      if port:get_direction() == "input" then channels = channels + 1 end
    end
    local p = node.properties
    local camera, purpose = policy.context(p)
    local chosen = policy.choose(selection, {
      channels = channels,
      role = p["media.role"],
      explicit_target = flags.has_defined_target,
      internal = p["uke.microphone.internal"] == "true",
      raw = p["uke.microphone.raw"] == "true",
      camera = camera,
      purpose = purpose,
    }, qualified(physical.properties))
    if not chosen or array_node() ~= physical then return end
    local entry = modules[chosen]
    if not entry or entry.serial ~= physical.properties["object.serial"] then return end
    local replacement = om:lookup {
      type = "SiLinkable",
      Constraint { "node.name", "=", "uke.mic.selected." .. chosen },
    }
    if replacement and lutils.canLink(props, replacement) then event:set_data("target", replacement) end
  end,
}:register()
