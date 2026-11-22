#!/usr/bin/env bash
# Host only; not executed during the source-only preparation session.
set -Eeuo pipefail
root=$(cd -- "$(dirname -- "$0")/.." && pwd)
work=$(mktemp -d)
trap 'rm -r -- "$work"' EXIT
"${CXX:-c++}" -std=c++17 -O2 -Wall -Wextra -Werror "$root/tests/battery-model.cpp" "$root/src/battery/checkpoint.cpp" -o "$work/model"
mkdir "$work/checkpoint"
"$work/model" "$work/checkpoint"
echo 'Battery model, observed EFC, atomic checkpoint and fixed-source arithmetic passed; physical acceptance remains open'
