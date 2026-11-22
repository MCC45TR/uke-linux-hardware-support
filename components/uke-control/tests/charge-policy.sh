#!/usr/bin/env bash
# Host-only synthetic inputs; no device discovery, SSH or power-supply access.
set -Eeuo pipefail
root=$(cd -- "$(dirname -- "$0")/.." && pwd)
work=$(mktemp -d)
trap 'rm -r -- "$work"' EXIT
"${CXX:-c++}" -std=c++20 -O2 -Wall -Wextra -Werror -I"$root/src" \
    "$root/tests/charge-policy.cpp" "$root/src/charge-policy.cpp" "$root/src/cli.cpp" $(pkg-config --cflags --libs libsystemd) -o "$work/policy"
"$work/policy"
printf '%s\n' 'Charging target/rate proposal and unsupported-apply contracts passed; HOST only'
