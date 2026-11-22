#!/usr/bin/env bash
set -Eeuo pipefail
cd "$(dirname "$0")/.."
mkdir -p build/tests
fixture=$(mktemp -d build/tests/admission.XXXXXX)
mkdir -p "$fixture/archive/qcom/sm7675"
printf 'Synthetic firmware bytes, never a device payload\n' > "$fixture/archive/qcom/sm7675/fixture.bin"
printf 'Synthetic host-fixture license text\n' > "$fixture/archive/LICENSE.fixture"
sha=$(sha256sum "$fixture/archive/qcom/sm7675/fixture.bin" | cut -d ' ' -f1)
license=$(sha256sum "$fixture/archive/LICENSE.fixture" | cut -d ' ' -f1)
jq -n --arg sha "$sha" --arg license "$license" '{schema_version:1,device:"uke",soc:"SM7675",redistribution_review_complete:true,firmware_blob_package_ready:true,admitted_files:[{source_path:"qcom/sm7675/fixture.bin",target_path:"qcom/sm7675/fixture.bin",sha256:$sha,source_commit:"0000000000000000000000000000000000000001",source_url:"https://example.invalid/fixture",source_profile:"fixture-profile",driver_compatible:"qcom,fixture",driver_request:"qcom/sm7675/fixture.bin",license:{name:"host-fixture-only",path:"LICENSE.fixture",sha256:$license,reviewed:true}}]}' > "$fixture/lock.json"
printf '%s\n' '{"schema_version":1,"device":"uke","requests":[{"path":"qcom/sm7675/fixture.bin","source_profile":"fixture-profile","driver_compatible":"qcom,fixture"}]}' > "$fixture/requests.json"
reject() { if bash src/audit-admission.sh "$1" "$fixture/archive" "$2" > "$fixture/rejected.log" 2>&1; then echo 'Invalid firmware admission unexpectedly accepted' >&2; exit 1; fi; }
bash src/audit-admission.sh "$fixture/lock.json" "$fixture/archive" "$fixture/requests.json"
reject manifests/firmware-lock.json "$fixture/requests.json"
jq '.admitted_files[0].license.reviewed=false' "$fixture/lock.json" > "$fixture/bad.json"
reject "$fixture/bad.json" "$fixture/requests.json"
jq '.admitted_files[0].source_path="../escape"' "$fixture/lock.json" > "$fixture/bad.json"
reject "$fixture/bad.json" "$fixture/requests.json"
jq '.requests[0].driver_compatible="qcom,foreign"' "$fixture/requests.json" > "$fixture/bad-requests.json"
reject "$fixture/lock.json" "$fixture/bad-requests.json"
printf 'Corrupt bytes\n' >> "$fixture/archive/qcom/sm7675/fixture.bin"
reject "$fixture/lock.json" "$fixture/requests.json"
printf '%s\n' 'Firmware admission fixtures passed; empty lock, missing license, traversal, foreign request and corrupt bytes rejected'
