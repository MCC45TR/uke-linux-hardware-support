#!/usr/bin/env bash
set -Eeuo pipefail
cd "$(dirname "$0")/.."
mkdir -p build/tests
fixture=$(mktemp -d build/tests/local-stage.XXXXXXXX)
trap 'rm -rf -- "$fixture"' EXIT
mkdir -p "$fixture/modem" "$fixture/gpu"
printf 'Synthetic local-only fixture\n' > "$fixture/modem/fixture.bin"
sha=$(sha256sum "$fixture/modem/fixture.bin" | cut -d ' ' -f1)
bytes=$(stat -c %s "$fixture/modem/fixture.bin")
jq -n --arg sha "$sha" --argjson bytes "$bytes" '{schema_version:1,device:"uke",soc:"SM7675",public_upload_allowed:false,redistribution_review_complete:false,files:[{source_group:"modem",source_path:"fixture.bin",target_path:"qcom/fixture.bin",bytes:$bytes,sha256:$sha}]}' > "$fixture/candidate.json"
stage() { bash src/stage-local.sh "$1" "$fixture/modem" "$fixture/gpu" "$2"; }
reject() { if stage "$1" "$2" > "$fixture/rejected.log" 2>&1; then echo 'Invalid local firmware input accepted' >&2; exit 1; fi; }
stage "$fixture/candidate.json" "$fixture/output"
cmp "$fixture/modem/fixture.bin" "$fixture/output/usr/lib/firmware/qcom/fixture.bin"
test -s "$fixture/output/LOCAL-ONLY.txt"
reject "$fixture/candidate.json" "$fixture/output"
jq '.files[0].target_path="../escape"' "$fixture/candidate.json" > "$fixture/bad.json"
reject "$fixture/bad.json" "$fixture/traversal"
test ! -e "$fixture/escape"
jq '.files[0].source_group="unknown"' "$fixture/candidate.json" > "$fixture/bad.json"
reject "$fixture/bad.json" "$fixture/unknown"
printf 'Corrupted bytes\n' >> "$fixture/modem/fixture.bin"
reject "$fixture/candidate.json" "$fixture/corrupt"
test ! -e "$fixture/corrupt"
printf '%s\n' 'Local firmware staging fixtures passed'
