#!/usr/bin/env bash
# Host-only audit; no target binary or donor script is executed.
set -Eeuo pipefail
[[ $# == 3 ]] || { echo 'Usage: audit-admission.sh LOCK_JSON ARCHIVE_ROOT ENABLED_DTB_REQUESTS_JSON' >&2; exit 2; }
lock=$1 archive=$(realpath "$2") requests=$3
for command in jq sha256sum realpath; do command -v "$command" >/dev/null || exit 2; done
jq -e '.schema_version==1 and .device=="uke" and .soc=="SM7675" and (.admitted_files|type)=="array"' "$lock" >/dev/null
jq -e '.schema_version==1 and .device=="uke" and (.requests|type)=="array"' "$requests" >/dev/null
jq -e '(.admitted_files|length)>0 and .redistribution_review_complete==true and .firmware_blob_package_ready==true' "$lock" >/dev/null || { echo 'No file-level firmware payload is admitted' >&2; exit 1; }
jq -e 'all(.admitted_files[];
  (.target_path|test("^[A-Za-z0-9_.+/-]+$")) and (.target_path|startswith("qcom/") or startswith("ath")) and
  (.target_path|contains("..")|not) and
  (.source_path|test("^[A-Za-z0-9_.+/-]+$")) and (.source_path|startswith("/")|not) and (.source_path|contains("..")|not) and
  (.sha256|test("^[a-f0-9]{64}$")) and (.source_commit|test("^[a-f0-9]{40}$")) and
  (.source_url|startswith("https://")) and (.source_profile|length)>0 and
  (.driver_compatible|length)>0 and .driver_request==.target_path and
  .license.reviewed==true and (.license.name|length)>0 and
  (.license.path|test("^[A-Za-z0-9_.+/-]+$")) and (.license.path|startswith("/")|not) and (.license.path|contains("..")|not) and
  (.license.sha256|test("^[a-f0-9]{64}$"))) and
  ([.admitted_files[].target_path]|length)==([.admitted_files[].target_path]|unique|length)' "$lock" >/dev/null || { echo 'Invalid firmware provenance or license entry' >&2; exit 1; }
while IFS=$'\t' read -r source target expected license license_sha profile compatible; do
    case ${target,,} in *persist*|*calib*|*secdata*|*tz*|*xbl*|*abl*|*hyp*|*keymaster*|*uefi*|*qsee*) echo 'Excluded firmware category' >&2; exit 1;; esac
    resolved=$(realpath -e "$archive/$source")
    resolved_license=$(realpath -e "$archive/$license")
    [[ $resolved == "$archive/"* && $resolved_license == "$archive/"* && ! -L $archive/$source && -f $resolved ]]
    [[ $(sha256sum "$resolved" | cut -d ' ' -f1) == "$expected" ]]
    [[ $(sha256sum "$resolved_license" | cut -d ' ' -f1) == "$license_sha" ]]
    jq -e --arg path "$target" --arg profile "$profile" --arg compatible "$compatible" 'any(.requests[]; .path==$path and .source_profile==$profile and .driver_compatible==$compatible)' "$requests" >/dev/null || { echo 'Firmware file has no matching enabled Uke driver request' >&2; exit 1; }
done < <(jq -r '.admitted_files[]|[.source_path,.target_path,.sha256,.license.path,.license.sha256,.source_profile,.driver_compatible]|@tsv' "$lock")
printf '%s\n' 'Firmware bytes, reviewed license identity and enabled Uke driver requests match'
