#!/bin/bash
# Compile only the new project policy fixture test; no upstream build system.
set -euo pipefail
root=$(git -C "$(dirname "$0")" rev-parse --show-toplevel)
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
g++ -std=c++20 -O2 -Wall -Wextra -Werror -UNDEBUG \
 -I"$root/uke-control/src" "$root/uke-control/tests/apt-policy.cpp" -o "$work/apt-policy"
"$work/apt-policy"
