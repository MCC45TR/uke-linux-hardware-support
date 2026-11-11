#!/usr/bin/bash
# COPR only: no real device operations; each child inherits an ioctl denial.
set -Eeuo pipefail
[[ ! -e /etc/senemos/kernel-update.conf && $# == 4 ]]
fixture=$(realpath "$1")
worker=$(realpath "$2")
adapter=$(realpath "$3")
mocks=$(realpath "$4")
sandbox=$(mktemp -d /tmp/uke-session.XXXXXXXX)
trap 'find "$sandbox" -depth -delete' EXIT
for mode in root sensors fallback failure invalid unscoped bad-socket missing-adapter; do
 mkdir "$sandbox/$mode"
 timeout 8 "$fixture" "$mode" "$sandbox/$mode" "$worker" "$adapter" "$mocks"
 [[ $(find "$sandbox/$mode" -mindepth 1 | wc -l) == 0 ]]
done
printf 'Eight original-library single-session cases passed.\n'
