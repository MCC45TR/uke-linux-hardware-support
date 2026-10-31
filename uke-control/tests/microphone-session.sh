#!/usr/bin/env bash
# COPR-only synthetic session. No physical device, audio file or network capture.
set -Eeuo pipefail
test ! -e /etc/senemos/kernel-update.conf
test ! -d /dev/snd
root=$(cd -- "$(dirname -- "$0")/.." && pwd)
stage=$(mktemp -d)
trap 'rm -rf -- "$stage"' EXIT
export XDG_RUNTIME_DIR=$stage/run XDG_CONFIG_HOME=$stage/config XDG_DATA_HOME=$stage/data XDG_STATE_HOME=$stage/state
mkdir -m700 "$XDG_RUNTIME_DIR"
mkdir -p "$XDG_CONFIG_HOME"/wireplumber/wireplumber.conf.d "$XDG_DATA_HOME"/wireplumber/scripts/lib "$XDG_CONFIG_HOME"/pipewire/pipewire.conf.d
cp "$root/packaging/96-uke-microphone-policy.conf" "$XDG_CONFIG_HOME/wireplumber/wireplumber.conf.d/"
cp "$root/src/uke-microphone.lua" "$XDG_DATA_HOME/wireplumber/scripts/"
cp "$root/src/uke-microphone-policy.lua" "$XDG_DATA_HOME/wireplumber/scripts/lib/"
cat > "$XDG_CONFIG_HOME/pipewire/pipewire.conf.d/90-fixture.conf" <<'CONFIG'
context.objects = [
 { factory = adapter args = {
   factory.name = audiotestsrc
   node.name = uke.fixture.array
   media.class = Audio/Source
   device.api = alsa
   alsa.card_name = POCO-Pad-X1
   alsa.components = "mic:uke-4ch-v4 kernel:7.3.1-106"
   audio.channels = 4
   audio.position = [ AUX0 AUX1 AUX2 AUX3 ]
   audio.rate = 48000
 } }
]
CONFIG
export UKE_MIC_FIXTURE_STAGE=$stage
dbus-run-session -- bash -Eeuo pipefail <<'SESSION'
stage=$UKE_MIC_FIXTURE_STAGE
pipewire >"$stage/pipewire.log" 2>&1 & pw_pid=$!
wireplumber >"$stage/wireplumber.log" 2>&1 & wp_pid=$!
rec_pid=
cleanup() {
 [[ -z $rec_pid ]] || kill "$rec_pid" 2>/dev/null || :
 kill "$wp_pid" "$pw_pid" 2>/dev/null || :
 wait "$wp_pid" "$pw_pid" 2>/dev/null || :
}
trap cleanup EXIT
trap 'cat "$stage/wireplumber.log" >&2; cat "$stage/pipewire.log" >&2' ERR
await_json() {
 local assertion=$1
 for ((attempt=0; attempt<100; ++attempt)); do
   if pw-dump > "$stage/graph.json" 2>/dev/null && jq -e "$assertion" "$stage/graph.json" >/dev/null; then return; fi
   sleep .1
 done
 printf 'Synthetic microphone assertion failed: %s\n' "$assertion" >&2
 return 1
}
await_json 'any(.[]; .type=="PipeWire:Interface:Metadata" and .props["metadata.name"]=="schema-sm-settings" and any(.metadata[]; .key=="uke.microphone.selection"))'
jq -e 'all(.[]; .info.props["uke.microphone.internal"]!=true and .info.props["uke.microphone.internal"]!="true")' "$stage/graph.json"
source_id=$(jq -r '.[]|select(.type=="PipeWire:Interface:Node" and .info.props["node.name"]=="uke.fixture.array")|.id' "$stage/graph.json")
wpctl set-default "$source_id"
for selection in uke-amic1 uke-amic3 uke-amic4 uke-amic5; do
 wpctl settings --save uke.microphone.selection "\"$selection\""
 await_json "any(.[]; .type==\"PipeWire:Interface:Node\" and .info.props[\"node.name\"]==\"uke.mic.selected.$selection\")"
 jq -e 'all(.[]; .type!="PipeWire:Interface:Node" or .info.props["node.name"]!="uke.fixture.array" or .info.state!="running")' "$stage/graph.json"
 pw-record --channels=2 --rate=48000 --format=s16 --properties='{ node.name = "uke.fixture.recorder" }' - > /dev/null 2>"$stage/recorder.log" & rec_pid=$!
 await_json "([.[]|select(.type==\"PipeWire:Interface:Node\" and .info.props[\"node.name\"]==\"uke.mic.selected.$selection\")|.id][0]) as \$selected | ([.[]|select(.type==\"PipeWire:Interface:Node\" and .info.props[\"node.name\"]==\"uke.fixture.recorder\")|.id][0]) as \$recorder | any(.[]; .type==\"PipeWire:Interface:Link\" and .info[\"output-node-id\"]==\$selected and .info[\"input-node-id\"]==\$recorder)"
 kill "$rec_pid"; wait "$rec_pid" || :; rec_pid=
 await_json 'all(.[]; .type!="PipeWire:Interface:Node" or .info.props["node.name"]!="uke.fixture.array" or .info.state!="running")'
done
wpctl settings --save uke.microphone.selection '"auto"'
await_json 'all(.[]; .info.props["uke.microphone.internal"]!=true and .info.props["uke.microphone.internal"]!="true")'
printf 'Isolated PipeWire/WirePlumber session: four manual routes, stereo capture, passive idle and auto teardown passed\n'
SESSION
