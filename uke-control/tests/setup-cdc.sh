#!/usr/bin/bash
# Native COPR only: no hardware or real service operation in this fixture.
set -Eeuo pipefail
[[ $# == 1 && $(uname -m) == aarch64 ]]
root=$(realpath "$1")
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
ulimit -c 0
for compiler in g++ clang++;do
 for mode in plain sanitized;do
  flags=()
  [[ $mode != sanitized ]] || flags=(-fsanitize=address,undefined -fno-omit-frame-pointer)
  "$compiler" -std=c++20 -O2 -g -Wall -Wextra -Werror "${flags[@]}" -I "$root/src" \
   $(pkg-config --cflags jsoncpp) "$root/tests/setup-cdc.cpp" $(pkg-config --libs jsoncpp) -o "$work/check"
  readelf -h "$work/check" | grep -F AArch64
  timeout 30s "$work/check"
 done
done
