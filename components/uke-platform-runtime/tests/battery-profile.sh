#!/usr/bin/env bash
# Prepared only. Uses synthetic private files and an injected host-test UID.
set -Eeuo pipefail
root=$(cd -- "$(dirname -- "$0")/.." && pwd)
work=$(mktemp -d)
trap 'rm -r -- "$work"' EXIT
pkg-config --atleast-version=257 libsystemd
"${CXX:-c++}" -std=c++17 -O2 -Wall -Wextra -Werror "$root/tests/battery-profile.cpp" "$root/src/battery/profile.cpp" "$root/src/battery/record.cpp" $(pkg-config --cflags --libs libsystemd) -o "$work/profile"
mkdir "$work/records"
"$work/profile" "$work/records"
echo 'Profile identity, schema, expiry, permissions and no-control boundaries passed; no hardware qualification'
