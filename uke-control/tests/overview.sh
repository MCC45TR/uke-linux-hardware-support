#!/usr/bin/env bash
# Native CLI with synthetic documents only; no system or hardware queries.
set -Eeuo pipefail
cd "$(dirname "$0")/.."
export LD_LIBRARY_PATH="$PWD/build${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
stage=$(mktemp -d)
trap 'rm -rf -- "$stage"' EXIT
app=$PWD/build/uke-control
cat >"$stage/before.json" <<'JSON'
{"schema":"org.uke.control.overview.v1","components":[{"component":"battery","availability":"available","observed_boottime_ns":1,"data":{"cycles":0,"text":"Türkçe 😀","secret":"TOKEN fixture@example.invalid /home/private 192.0.2.1 00:11:22:33:44:55 SERIAL-PRIVATE"}},{"component":"usb","availability":"error","observed_boottime_ns":1,"data":null}]}
JSON
"$app" overview --fixture "$stage/before.json" --json >"$stage/fixture.json"
jq -e '.simulation==true and .physical_acceptance==null and .components[0].data.cycles==0 and .components[0].data.text=="Türkçe 😀"' "$stage/fixture.json" >/dev/null
"$app" support export "$stage/before.json" "$stage/public.json" --json >"$stage/export.json"
test "$(stat -c %a "$stage/public.json")" = 600
jq -e '.schema=="org.uke.control.support.v1" and (.components|length)==2 and .physical_acceptance==null' "$stage/public.json" >/dev/null
! grep -Eq 'TOKEN|example.invalid|/home/|192\.0\.2\.1|00:11:22|SERIAL|secret|Türkçe' "$stage/public.json"
cp "$stage/public.json" "$stage/unchanged"
if "$app" support export "$stage/before.json" "$stage/public.json" --json >"$stage/out" 2>"$stage/error"; then exit 1; fi
cmp "$stage/public.json" "$stage/unchanged"
jq '.components[0].observed_boottime_ns=99 | .components[1].observed_boottime_ns=99' "$stage/before.json" >"$stage/same.json"
"$app" compare "$stage/before.json" "$stage/same.json" --json | jq -e '(.changes|length)==0 and .causality_established==false and .after_observations[0].observed_boottime_ns==99' >/dev/null
jq '.components[0].data.cycles=1 | del(.components[1])' "$stage/same.json" >"$stage/after.json"
"$app" compare "$stage/before.json" "$stage/after.json" --json >"$stage/diff.json"
jq -e 'any(.changes[];.path=="/battery/data/cycles" and .before==0 and .after==1) and any(.changes[];.path=="/usb/data" and .before_present==true and .after_present==false)' "$stage/diff.json" >/dev/null
ln -s "$stage/before.json" "$stage/link"
printf '%s\n' '{"schema":"org.uke.control.overview.v1","schema":"duplicate","components":[]}' >"$stage/duplicate.json"
for input in "$stage/link" "$stage/duplicate.json"; do
    if "$app" overview --fixture "$input" --json >"$stage/out" 2>"$stage/error"; then exit 1; fi
    test ! -s "$stage/out"
done
if "$app" overview --fixture "$stage/before.json" usb role host >"$stage/out" 2>"$stage/error"; then exit 1; fi
if "$app" roots inspect '../../device' >"$stage/out" 2>"$stage/error"; then exit 1; fi
"$app" capabilities --json | jq -e '.metadata_only==true and .hardware_controls_qualified==null and (.read_only_components|length)==15' >/dev/null
printf '%s\n' 'Native snapshot fixtures, Unicode/zero, bounded comparison, private files, allowlist export and no-action refusal paths passed'
