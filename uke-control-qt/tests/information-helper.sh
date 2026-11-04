#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
set -euo pipefail
helper=$1
if "$helper" /etc/shadow; then exit 1; else test "$?" = 64; fi
if "$helper" charging ignored; then exit 1; else test "$?" = 64; fi
# COPR build hosts have no tablet provider. Inventory reads remain bounded,
# read-only and successful even when no devices are present.
output=$("$helper" dock)
[[ "$output" == *'"schema":"org.uke.qt.kernel-information.v1"'* ]]
[[ "$output" == *'"devices":['* ]]
[[ "$output" == *'"kernel_managed":'* ]]
