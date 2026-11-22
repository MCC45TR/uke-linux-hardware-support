#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
set -Eeuo pipefail
program=build/uke-control-request
"$program" --help | grep -q 'terminal companion'
stage=$(mktemp -d)
trap 'rm -rf -- "$stage"' EXIT
refuse() {
    local expected=$1
    shift
    local status=0
    "$program" "$@" </dev/null >"$stage/out" 2>"$stage/error" || status=$?
    [[ $status == "$expected" && -s $stage/error ]]
}
refuse 64 arbitrary.command
refuse 64 battery.status
refuse 64 charge.mode mode=invalid
refuse 64 charge.mode mode=firmware mode=firmware
# Valid terminal action still cannot run as root or without a real terminal.
refuse 1 charge.mode mode=firmware
printf '%s\n' 'Terminal companion validation and noninteractive no-execution refusals passed'
