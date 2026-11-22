#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
set -Eeuo pipefail
program=${UKE_TUI_TEST_PROGRAM:-build/uke-control-tui}
"$program" --help | grep -q 'terminal interface'
"$program" --version | grep -q '^uke-control-tui '
stage=$(mktemp -d)
trap 'rm -rf -- "$stage"' EXIT
if "$program" --language en </dev/null >"$stage/out" 2>"$stage/error"; then
    printf '%s\n' 'TUI unexpectedly accepted redirected input.' >&2
    exit 1
fi
grep -q 'interactive terminal' "$stage/error"
! LC_ALL=C grep -q $'\033' "$stage/out"
printf '%s\n' 'Non-TTY help/version and clean terminal refusal passed'
