#!/usr/bin/bash
# SPDX-License-Identifier: MIT
# COPR source worker only. No native compilation or production policy load.
set -Eeuo pipefail
export LC_ALL=C
[[ $# == 1 && ${UKE_COPR_SOURCE_WORKER:-} == 1 && $EUID != 0 && ! -e /etc/senemos/kernel-update.conf ]]
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
contract=$owner/packaging/motion-integration.json
result=$(realpath -m -- "$1")
mkdir -p "$result"
work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT
parent_url=$(jq -er .parent.url "$contract")
policy_url=$(jq -er .policy_fixture.url "$contract")
[[ $parent_url == https://github.com/MCC45TR/uke-linux-hardware-support/releases/download/* && $policy_url == https://dl.fedoraproject.org/* ]]
curl -qfsSL --proto '=https' --max-filesize 16777216 --max-time 120 "$parent_url" -o "$work/parent.src.rpm"
curl -qfsSL --proto '=https' --max-filesize 8388608 --max-time 120 "$policy_url" -o "$work/original-policy.rpm"
bash "$owner/packaging/prepare-control-motion.sh" "$work/parent.src.rpm" "$work/original-policy.rpm" "$work/sources"
mkdir -p "$work/top/BUILD" "$work/top/BUILDROOT" "$work/top/RPMS" "$work/top/SPECS"
rpmbuild -bs --target aarch64 --define 'dist .fc46' --define "_topdir $work/top" \
 --define "_sourcedir $work/sources" --define "_srcrpmdir $result" "$work/sources/uke-control.spec"
