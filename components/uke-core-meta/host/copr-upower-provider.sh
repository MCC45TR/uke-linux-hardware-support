#!/usr/bin/bash
# COPR source worker only; no new package name or provider replacement script.
set -Eeuo pipefail
[[ $# == 1 && ! -e /sys/firmware/devicetree/base/model ]]
command -v rpmspec >/dev/null
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
curl -qfsSL --max-time 90 "$(jq -er .parent_srpm_url "$owner/manifests/original-upower-provider.json")" -o "$work/parent.src.rpm"
bash "$owner/host/prepare-upower-provider.sh" "$work/parent.src.rpm" "$1"
