#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
set -euo pipefail
app=${1:?application path required}
export QT_QPA_PLATFORM=offscreen
"$app" --help >/dev/null
"$app" --version >/dev/null
set +e
"$app" --fixture /dev/null >/dev/null 2>&1
status=$?
set -e
[[ $status -eq 64 ]] || { echo "Non-regular fixture was not rejected" >&2; exit 1; }
if [[ $(id -u) -eq 0 ]]; then
    set +e
    "$app" --start charge.status >/dev/null 2>&1
    status=$?
    set -e
    [[ $status -eq 77 ]] || { echo "Root GUI was not refused" >&2; exit 1; }
fi
