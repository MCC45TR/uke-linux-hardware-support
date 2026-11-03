-- SPDX-License-Identifier: MIT
-- Pure control decisions; never called from an audio processing callback.
local M = {}
M.channels = { ["uke-amic1"] = "AUX0", ["uke-amic3"] = "AUX1",
               ["uke-amic4"] = "AUX2", ["uke-amic5"] = "AUX3" }
function M.qualified(p, positions)
  return p["media.class"] == "Audio/Source" and p["device.api"] == "alsa"
    and (p["alsa.card_name"] == "POCO-Pad-X1" or p["alsa.long_card_name"] == "Xiaomi Pad 7")
    and type(p["alsa.components"]) == "string"
    and p["alsa.components"]:match("^mic:uke%-4ch%-v4 kernel:%d+%.%d+%.%d+%-%d+$") ~= nil
    and tonumber(p["audio.channels"]) == 4 and type(positions) == "table"
    and #positions == 4 and positions[1] == "AUX0" and positions[2] == "AUX1"
    and positions[3] == "AUX2" and positions[4] == "AUX3"
end
function M.choose(selection, stream, qualified)
  if not qualified then return nil, "unqualified_array" end
  if stream.explicit_target then return nil, "explicit_target_preserved" end
  if stream.internal or stream.raw or stream.role == "Production"
    or (stream.channels ~= 1 and stream.channels ~= 2) then
    return nil, "raw_or_unknown_stream_preserved"
  end
  if M.channels[selection] then return selection, "manual" end
  if selection ~= "auto" then return nil, "invalid_preference" end
  -- shortcut: profiles remain uncalibrated; enable only after acoustic acceptance.
  return nil, "uncalibrated_auto_keeps_raw"
end
function M.speech(profile, selection, stream, qualified)
  if not qualified then return nil, "unqualified_array" end
  if stream.explicit_target or stream.internal or stream.raw or stream.dont_filter
      or stream.role == "Production" or (stream.channels ~= 1 and stream.channels ~= 2) then
    return nil, "explicit_or_raw_stream_preserved"
  end
  if profile ~= "calls" and profile ~= "all" then return nil, "processing_off_or_unknown" end
  if profile == "calls" and stream.role ~= "Communication" and stream.purpose ~= "call"
      and stream.purpose ~= "narration" then return nil, "non_speech_stream_preserved" end
  if M.channels[selection] then return selection, "speech_manual_channel" end
  -- A deterministic reference channel, not an acoustically optimized automatic choice.
  if selection == "auto" then return "uke-amic1", "speech_reference_channel" end
  return nil, "invalid_preference"
end
function M.context(p)
  local camera = p["uke.microphone.camera"]
  local purpose = p["uke.microphone.purpose"]
  if camera ~= "front" and camera ~= "rear" and camera ~= "none" then camera = "unknown" end
  if purpose ~= "call" and purpose ~= "scene" and purpose ~= "narration" and purpose ~= "recording" then
    purpose = p["media.role"] == "Communication" and "call" or "unknown"
  end
  return camera, purpose
end
return M
