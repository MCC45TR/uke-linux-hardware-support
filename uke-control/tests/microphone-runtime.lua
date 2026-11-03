-- SPDX-License-Identifier: MIT
-- Control API fixture only. No sound server, devices, samples or files.
local pure = dofile(arg[1])
package.preload["uke-microphone-policy"] = function() return pure end
local current_event
package.preload["linking-utils"] = function() return {
  unwrap_select_target_event=function() return table.unpack(current_event,1,6) end,
  canLink=function() return true end,
} end
local callbacks,objects,hooks,subscriptions={},{},{},{}
local selection,profile,agc,allocations,unloads,arguments,fail="auto","off",false,0,0,nil,false
function Interest(v) return v end
function EventInterest(v) return v end
function Constraint(v) return v end
function ObjectManager() return {
  connect=function(_,name,cb) callbacks[name]=cb end,activate=function() end,
  iterate=function() local i=0;return function() i=i+1;return objects[i] end end,
} end
Json={
  Raw=function() return {parse=function() return {"AUX0","AUX1","AUX2","AUX3"} end} end,
  Array=function(v) return v end,
  Object=function(v) v.get_data=function() arguments=v;return "fixture" end;return v end,
}
Log={open_topic=function() return {warning=function() end} end}
Settings={
  get_string=function(key) if key=="uke.microphone.selection" then return selection else return profile end end,
  get_boolean=function() return agc end,
  subscribe=function(key,cb) subscriptions[key]=cb end,
}
local function update() subscriptions["uke.microphone.selection"]() end
function SimpleEventHook(h) h.register=function() hooks[h.name]=h end;return h end
local last_library
function LocalModule(name)
  if fail then error("fixture unavailable backend") end
  last_library=name
  local capture=arguments["capture.props"]
  assert(capture["node.passive"] and capture["node.dont-fallback"] and capture["stream.dont-remix"])
  assert(capture["target.object"]=="81")
  local source
  if name=="libpipewire-module-loopback" then source=arguments["playback.props"]
  else
    assert(name=="libpipewire-module-echo-cancel")
    assert(arguments["library.name"]=="aec/libspa-aec-webrtc" and arguments["monitor.mode"])
    assert(arguments["aec.args"]["webrtc.high_pass_filter"] and arguments["aec.args"]["webrtc.noise_suppression"])
    assert(arguments["aec.args"]["webrtc.gain_control"]==agc)
    assert(arguments["sink.props"]["node.passive"] and arguments["sink.props"]["stream.capture.sink"])
    assert(arguments["sink.props"]["audio.position"][2]=="FR")
    source=arguments["source.props"]
  end
  assert(source["media.class"]=="Audio/Source/Internal" and source["node.hidden"])
  assert(arguments["audio.position"][1]=="MONO")
  allocations=allocations+1
  return {unload=function() unloads=unloads+1 end}
end
dofile(arg[2]);assert(allocations==0)
local raw={properties={["media.class"]="Audio/Source",["device.api"]="alsa",
  ["alsa.card_name"]="POCO-Pad-X1",["alsa.components"]="mic:uke-4ch-v4 kernel:7.3.1-1500",
  ["audio.channels"]="4",["audio.position"]="fixture",["object.serial"]="81"}}
objects={raw};callbacks["object-added"](nil,raw);assert(allocations==0)
selection="uke-amic4";update();assert(allocations==1 and arguments["capture.props"]["audio.position"][1]=="AUX2")
update();assert(allocations==1)
local state,changed="running",nil
local internal={properties={["uke.microphone.internal"]=true,["uke.microphone.id"]="uke-amic4",
  ["uke.microphone.key"]="manual:uke-amic4",["uke.microphone.stage"]="source"},
  get_state=function() return state end,connect=function(_,_,cb) changed=cb end}
objects={raw,internal};callbacks["object-added"](nil,internal)
local port={get_direction=function() return "input" end}
local stream={properties={},iterate_ports=function() local i=0;return function() i=i+1;if i<=2 then return port end end end}
local replacement={}
local om={lookup=function() return replacement end}
local target={get_associated_proxy=function() return raw end}
local si={get_associated_proxy=function() return stream end}
local event={set_data=function(self,_,value) self.target=value end}
current_event={false,om,si,{["media.class"]="Stream/Input/Audio"},{},target}
local hook=hooks["linking/uke-microphone-selection"]
hook.execute(event);assert(event.target==replacement)
for _,flag in ipairs {"has_defined_target","was_handled","peer_id"} do
  event.target=nil;current_event[5]={[flag]=true};hook.execute(event);assert(event.target==nil)
end
current_event[5]={}
selection="auto";update();assert(unloads==0)
state="idle";changed();assert(unloads==1)
objects={raw};profile="calls";update();assert(allocations==2 and last_library=="libpipewire-module-echo-cancel")
assert(arguments["capture.props"]["audio.position"][1]=="AUX0")
assert(arguments["source.props"]["uke.microphone.effects"]=="aec,ns,hpf")
event.target=nil;hook.execute(event);assert(event.target==nil)
stream.properties["media.role"]="Communication";hook.execute(event);assert(event.target==replacement)
event.target=nil;stream.properties["uke.microphone.raw"]=true;hook.execute(event);assert(event.target==nil)
stream.properties["uke.microphone.raw"]=nil
stream.properties["node.dont-filter"]=true;hook.execute(event);assert(event.target==nil)
stream.properties["node.dont-filter"]=nil
agc=true;update();assert(allocations==3 and unloads==2)
assert(arguments["source.props"]["uke.microphone.effects"]=="aec,ns,hpf,agc")
profile="off";update();assert(unloads==3)
profile="all";fail=true;update();assert(allocations==3)
event.target=nil;hook.execute(event);assert(event.target==nil) -- failure keeps ordinary target
fail=false;update();assert(allocations==4)
objects={};callbacks["object-removed"](nil,raw);assert(unloads==4)
selection="invalid";objects={raw};update();assert(allocations==4)
print("Passive manual/speech module routing, calls/all, digital AGC, explicit/raw precedence, failure fallback and deferred teardown passed")
