-- SPDX-License-Identifier: MIT
-- Mock the control API only; this fixture never connects to PipeWire or ALSA.
local pure = dofile(arg[1])
package.preload["uke-microphone-policy"] = function() return pure end
local current_event
package.preload["linking-utils"] = function() return {
  unwrap_select_target_event = function() return table.unpack(current_event, 1, 6) end,
  canLink = function() return true end,
} end
local callbacks, objects, hooks, settings_callback = {}, {}, {}, nil
local selection, allocations, unloads, arguments = "auto", 0, 0, nil
function Interest(value) return value end
function EventInterest(value) return value end
function Constraint(value) return value end
function ObjectManager()
  return {
    connect = function(_, name, callback) callbacks[name] = callback end,
    activate = function() end,
    iterate = function() local i=0; return function() i=i+1; return objects[i] end end,
  }
end
Json = {
  Raw = function() return { parse = function() return {"AUX0","AUX1","AUX2","AUX3"} end } end,
  Array = function(value) return value end,
  Object = function(value) value.get_data = function() arguments = value; return "fixture" end; return value end,
}
Log = { open_topic = function() return {warning=function() end} end }
Settings = {
  get_string = function() return selection end,
  subscribe = function(_, callback) settings_callback = callback end,
}
function SimpleEventHook(hook) hook.register=function() hooks[hook.name]=hook end; return hook end
function LocalModule(name)
  assert(name == "libpipewire-module-loopback")
  assert(arguments["capture.props"]["node.passive"] == true)
  assert(arguments["capture.props"]["node.dont-fallback"] == true)
  assert(arguments["capture.props"]["stream.dont-remix"] == true)
  assert(arguments["playback.props"]["media.class"] == "Audio/Source/Internal")
  assert(arguments["playback.props"]["node.hidden"] == true)
  assert(arguments["audio.position"][1] == "MONO")
  allocations = allocations + 1
  return {unload=function() unloads=unloads+1 end}
end
dofile(arg[2])
assert(allocations == 0)
local raw = {properties={ ["media.class"]="Audio/Source", ["device.api"]="alsa",
  ["alsa.card_name"]="POCO-Pad-X1", ["alsa.components"]="mic:uke-4ch-v4 kernel:7.3.1-106",
  ["audio.channels"]="4", ["audio.position"]="fixture", ["object.serial"]="81" }}
objects = {raw}
callbacks["object-added"](nil,raw)
assert(allocations == 0) -- Auto never allocates an audio module.
selection="uke-amic4"; settings_callback()
assert(allocations == 1 and arguments["capture.props"]["audio.position"][1] == "AUX2")
settings_callback(); assert(allocations == 1)
local state, changed = "running", nil
local internal = {
  properties={["uke.microphone.internal"]="true",["uke.microphone.id"]="uke-amic4"},
  get_state=function() return state end,
  connect=function(_,_,cb) changed=cb end,
}
objects={raw,internal}; callbacks["object-added"](nil,internal)
local port={get_direction=function() return "input" end}
local stream_node={properties={},iterate_ports=function() local i=0; return function() i=i+1; if i<=2 then return port end end end}
local replacement={}
local target={get_associated_proxy=function() return raw end}
local si={get_associated_proxy=function() return stream_node end}
local om={lookup=function() return replacement end}
local event={set_data=function(self,_,value) self.target=value end}
current_event={false,om,si,{["media.class"]="Stream/Input/Audio"},{},target}
local hook=hooks["linking/uke-microphone-selection"]
hook.execute(event); assert(event.target==replacement)
event.target=nil; current_event[5]={has_defined_target=true}
hook.execute(event); assert(event.target==nil)
current_event[5]={was_handled=true}; hook.execute(event); assert(event.target==nil)
selection="auto"; settings_callback(); assert(unloads==0) -- Running recording survives rollback.
state="idle"; changed(); assert(unloads==1)
selection="invalid"; settings_callback(); assert(allocations==1)
objects={}; callbacks["object-removed"](nil,raw)
print("Zero-auto allocation, passive routing, explicit/raw precedence and deferred teardown passed")
