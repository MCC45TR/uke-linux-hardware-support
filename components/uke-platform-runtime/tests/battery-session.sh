#!/usr/bin/env bash
# Host only. Prepared source; not executed during the no-test implementation.
set -Eeuo pipefail
root=$(cd -- "$(dirname -- "$0")/.." && pwd)
work=$(mktemp -d)
trap 'rm -r -- "$work"' EXIT
"${CXX:-c++}" -std=c++17 -O2 -Wall -Wextra -Werror "$root/tests/battery-session.cpp" "$root/src/battery/session.cpp" "$root/src/battery/checkpoint.cpp" -o "$work/session"
mkdir "$work/state"
"$work/session" "$work/state"
echo 'Battery session qualification, restart, fault, coverage and storage contracts passed; no hardware result'
