#!/usr/bin/bash
# COPR source worker; reuse all accepted Control14 inputs and checks.
set -Eeuo pipefail
[[ $# == 1 && ! -e /etc/senemos/kernel-update.conf ]]
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
workspace=$(dirname "$owner")
[[ -z $(git -C "$workspace" status --porcelain) ]]
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
curl -qfsSL --retry 2 --max-time 120 "$(jq -er .parent.url "$owner/packaging/selinux-integration.json")" -o "$work/parent.src.rpm"
bash "$owner/packaging/prepare-control-selinux.sh" "$work/parent.src.rpm" "$1"
