#!/bin/bash
# Validate this one attributed shared preset; never infer rights for other blobs.
set -euo pipefail
root=$(git -C "$(dirname -- "$0")" rev-parse --show-toplevel)
source=${1:?Extracted speaker source directory required}
bash "$root/xiaomi-uke-firmware/src/audit-speaker-source.sh" "$source/fs19xx.fsm" "$source/odm-NOTICE.xml.gz" "$source"
