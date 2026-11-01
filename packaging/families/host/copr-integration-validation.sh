#!/usr/bin/env bash
# COPR source worker only; solver compilation must not run on a tablet.
set -Eeuo pipefail
root=$(cd -- "$(dirname -- "$0")/../../.."; pwd)
for id in $(jq -er '.builds[].id' "$root/packaging/families/consolidation/builds.json"); do
    result=$(curl --fail --silent --show-error --max-time 40 "https://copr.fedorainfracloud.org/api_3/build/$id")
    jq -e '.state=="succeeded" and .chroots==["fedora-rawhide-aarch64"]' <<<"$result" >/dev/null
done
UKE_COPR_INTEGRATION_FIXTURE=1 bash "$root/packaging/families/tests/integration-solver.sh"
bash "$root/packaging/families/tests/service-rename.sh"
