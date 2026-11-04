#!/usr/bin/env bash
# COPR only: native injected clocks/counters and real temporary state files.
set -Eeuo pipefail
test ! -e /etc/senemos/kernel-update.conf
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
stage=$(mktemp -d)
trap 'find "$stage" -depth -delete' EXIT
export ASAN_OPTIONS=halt_on_error=1:abort_on_error=1:detect_leaks=1
export UBSAN_OPTIONS=halt_on_error=1:abort_on_error=1
for compiler in g++ clang++; do
 for sanitize in no yes; do
  flags=()
  [[ $sanitize == no ]] || flags+=(-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer)
  "$compiler" -std=c++20 -O2 -UNDEBUG -Wall -Wextra -Werror "${flags[@]}" \
   "$owner/tests/rtc-counter-offset.cpp" -o "$stage/check"
  "$stage/check"
  printf 'Counter UTC native contract: %s sanitizers=%s passed\n' "$compiler" "$sanitize"
 done
done
