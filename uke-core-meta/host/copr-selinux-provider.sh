#!/usr/bin/bash
# COPR source worker; no policy code is rebuilt in this package.
set -Eeuo pipefail
[[ $# == 1 && ! -e /sys/firmware/devicetree/base/model ]]
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
curl -qfsSL --retry 2 --max-time 120 "$(jq -er .parent_srpm_url "$owner/manifests/original-selinux-provider.json")" -o "$work/parent.src.rpm"
bash "$owner/host/prepare-selinux-provider.sh" "$work/parent.src.rpm" "$1"
