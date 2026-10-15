#!/usr/bin/env bash
# Host-only byte selection; no download, device access or publication.
set -Eeuo pipefail
[[ $# == 4 ]] || { echo 'Usage: stage-local.sh CANDIDATE_JSON MODEM_FILES GPU_FILES OUTPUT' >&2; exit 2; }
lock=$1 modem=$(realpath -e "$2") gpu=$(realpath -e "$3") output=$4
[[ -d $modem && -d $gpu && ! -e $output && ! -L $output ]] || { echo 'Input directories must exist and output must be new' >&2; exit 1; }
jq -e '.schema_version==1 and .device=="uke" and .soc=="SM7675" and
  .public_upload_allowed==false and .redistribution_review_complete==false and
  (.files|length)>0 and
  all(.files[];
    (.source_group=="modem" or .source_group=="gpu") and
    (.source_path|test("^[A-Za-z0-9_+.-]+(/[A-Za-z0-9_+.-]+)*$")) and
    (.target_path|test("^[A-Za-z0-9_+.-]+(/[A-Za-z0-9_+.-]+)*$")) and
    (.source_path|split("/")|all(.!="." and .!="..")) and
    (.target_path|split("/")|all(.!="." and .!="..")) and
    (.sha256|test("^[a-f0-9]{64}$")) and (.bytes|type)=="number" and .bytes>0 and (.bytes|floor)==.bytes) and
  ([.files[].target_path]|length)==([.files[].target_path]|unique|length)' "$lock" >/dev/null
parent=$(dirname "$output")
[[ -d $parent ]] || { echo 'Output parent must exist' >&2; exit 1; }
stage=$(mktemp -d "$parent/.firmware-stage.XXXXXXXX")
trap 'rm -rf -- "$stage"' EXIT
while IFS=$'\t' read -r group source target bytes expected; do
    root=$modem
    [[ $group == modem ]] || root=$gpu
    resolved=$(realpath -e "$root/$source")
    [[ $resolved == "$root/"* && -f $resolved && ! -L $root/$source ]]
    [[ $(stat -c %s "$resolved") == "$bytes" && $(sha256sum "$resolved" | cut -d ' ' -f1) == "$expected" ]] || { echo "Firmware identity mismatch: $target" >&2; exit 1; }
    install -Dm644 -- "$resolved" "$stage/usr/lib/firmware/$target"
done < <(jq -r '.files[]|[.source_group,.source_path,.target_path,.bytes,.sha256]|@tsv' "$lock")
install -Dm644 -- "$lock" "$stage/local-firmware-candidates.json"
printf '%s\n' 'Local evaluation payload. Redistribution is unreviewed. Do not upload this directory or an archive containing it to COPR or a public release.' > "$stage/LOCAL-ONLY.txt"
mv -T -- "$stage" "$output"
trap - EXIT
printf '%s\n' 'Pinned local firmware staged; public firmware admission remains closed'
