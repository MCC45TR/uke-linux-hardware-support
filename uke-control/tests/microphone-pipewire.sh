#!/usr/bin/env bash
# COPR only: isolated sound server, synthetic zero streams, no ALSA or hardware.
set -Eeuo pipefail
test ! -e /etc/senemos/kernel-update.conf
stage=$(mktemp -d)
pids=()
cleanup() { for pid in "${pids[@]}"; do kill "$pid" 2>/dev/null || :; done; wait 2>/dev/null || :; rm -r -- "$stage"; }
trap cleanup EXIT
export XDG_RUNTIME_DIR=$stage/runtime
mkdir -m700 "$XDG_RUNTIME_DIR" "$stage/config"
cat > "$stage/config/test.conf" <<'CONF'
context.properties = { core.daemon = true core.name = pipewire-0 default.clock.rate = 48000 }
context.spa-libs = { audio.convert.* = audioconvert/libspa-audioconvert support.* = support/libspa-support }
context.modules = [
 { name = libpipewire-module-protocol-native }
 { name = libpipewire-module-access args = { access.force = unrestricted } }
 { name = libpipewire-module-spa-node-factory }
 { name = libpipewire-module-client-node }
 { name = libpipewire-module-adapter }
 { name = libpipewire-module-link-factory }
 { name = libpipewire-module-metadata }
 { name = libpipewire-module-echo-cancel args = {
   library.name = aec/libspa-aec-webrtc monitor.mode = true audio.rate = 48000 audio.position = [ MONO ]
   aec.args = { webrtc.high_pass_filter = true webrtc.noise_suppression = true webrtc.gain_control = false }
   capture.props = { node.autoconnect = false node.name = fixture.capture audio.position = [ AUX0 ] audio.rate = 48000 stream.dont-remix = true node.passive = true node.dont-fallback = true }
   source.props = { node.name = fixture.speech media.class = Audio/Source/Internal audio.position = [ MONO ] audio.rate = 48000 node.hidden = true }
   sink.props = { node.autoconnect = false node.name = fixture.reference audio.position = [ FL FR ] audio.rate = 48000 node.passive = true stream.capture.sink = true stream.monitor = true }
 } }
]
context.objects = [
 { factory = spa-node-factory args = { factory.name = support.node.driver node.name = Fixture-Driver node.group = pipewire.dummy priority.driver = 20000 } }
 { factory = adapter args = { factory.name = support.null-audio-sink node.name = fixture.sink media.class = Audio/Sink audio.position = [ FL FR ] object.linger = true adapter.auto-port-config = { mode = dsp monitor = true position = preserve } } }
]
CONF
PIPEWIRE_CONFIG_DIR="$stage/config" pipewire -c test.conf > "$stage/server.log" 2>&1 & pids+=("$!")
for i in {1..60}; do [[ -S $XDG_RUNTIME_DIR/pipewire-0 ]] && break; sleep .1; done
[[ -S $XDG_RUNTIME_DIR/pipewire-0 ]] || { cat "$stage/server.log"; exit 1; }
mkdir -p "$stage/config/wireplumber/wireplumber.conf.d"
cat > "$stage/config/wireplumber/wireplumber.conf.d/90-fixture.conf" <<'WP'
wireplumber.profiles = {
 main = { monitor.alsa = disabled monitor.bluez = disabled monitor.bluez-midi = disabled
          monitor.libcamera = disabled monitor.v4l2 = disabled }
}
WP
dbus-run-session -- env XDG_CONFIG_HOME="$stage/config" wireplumber > "$stage/wp.log" 2>&1 & pids+=("$!")
pw-cat --playback --raw --rate 48000 --channels 4 --channel-map AUX0,AUX1,AUX2,AUX3 --format f32 --target 0 --properties '{ node.name = fixture.raw media.class = Audio/Source node.autoconnect = false }' /dev/zero > "$stage/raw.log" 2>&1 & pids+=("$!")
wait_port() { for i in {1..60}; do pw-link "$1" | grep -Fq "$2" && return; sleep .1; done; pw-link -o; pw-link -i; cat "$stage/server.log" "$stage/wp.log" "$stage/raw.log"; return 1; }
wait_port -o 'fixture.raw:capture_AUX0'
wait_port -i 'fixture.capture:input_AUX0'
pw-link -P fixture.raw:capture_AUX0 fixture.capture:input_AUX0
pw-link -P fixture.sink:monitor_FL fixture.reference:input_FL
pw-link -P fixture.sink:monitor_FR fixture.reference:input_FR
# Input-only speech must continue producing frames while the output sink is idle.
record() {
 local name=$1
 timeout 12s pw-cat --record --raw --rate 48000 --channels 1 --channel-map MONO --format f32 --target 0 --properties "{ node.name = $name node.autoconnect = false }" --sample-count 48000 - > "$stage/$name.raw" 2> "$stage/$name.log" &
 local client=$!;pids+=("$client")
 wait_port -i "$name:input_MONO"
 pw-link fixture.speech:capture_MONO "$name:input_MONO"
 if ! wait "$client"; then cat "$stage/$name.log" "$stage/server.log"; return 1; fi
 test "$(wc -c < "$stage/$name.raw")" -eq 192000
 rm "$stage/$name.raw"
}
record fixture.record-only
pw-cat --playback --raw --rate 48000 --channels 2 --format f32 --target 0 --properties '{ node.name = fixture.render node.autoconnect = false }' /dev/zero > "$stage/render.log" 2>&1 & pids+=("$!")
wait_port -o 'fixture.render:output_FL'
pw-link fixture.render:output_FL fixture.sink:playback_FL
pw-link fixture.render:output_FR fixture.sink:playback_FR
record fixture.duplex
printf '%s\n' 'Real PipeWire/WebRTC module: 48000 synthetic mono frames with idle render and active stereo render; no hardware involved'
