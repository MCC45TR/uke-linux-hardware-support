#!/usr/bin/bash
# COPR only; controlled user files, no real desktop/hardware operation.
set -Eeuo pipefail
test ! -e /etc/senemos/kernel-update.conf
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
ulimit -c 0
for cc in g++ clang++;do
 for sanitize in 0 1;do
  flags=(-std=c++20 -O1 -g -UNDEBUG -Wall -Wextra -Werror -fno-omit-frame-pointer)
  [[ $sanitize == 0 ]] || flags+=(-fsanitize=address,undefined -fno-sanitize-recover=all)
  "$cc" "${flags[@]}" $(pkg-config --cflags jsoncpp libsystemd) "$owner/tests/brightness-curve.cpp" \
   $(pkg-config --libs jsoncpp libsystemd) -o "$work/test"
  ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 "$work/test"
  printf 'OSS brightness curve actual callbacks passed:%s/sanitize%s; no device acceptance.\n' "$cc" "$sanitize"
 done
done
