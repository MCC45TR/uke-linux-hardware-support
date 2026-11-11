#!/usr/bin/bash
# Native COPR only; the actual probe reads controlled temporary files.
set -Eeuo pipefail
test ! -e /etc/senemos/kernel-update.conf
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
ulimit -c 0
for compiler in g++ clang++;do
 for sanitizer in 0 1;do
  flags=(-std=c++20 -O2 -UNDEBUG -Wall -Wextra -Werror)
  if [[ $sanitizer == 1 ]];then flags+=(-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer);fi
  "$compiler" "${flags[@]}" "$owner/tests/critical-health.cpp" -o "$work/critical"
  "$work/critical"
  "$compiler" "${flags[@]}" -Wno-unused-function "$owner/tests/critical-userdata.cpp" -o "$work/userdata" $(pkg-config --cflags --libs fdisk zlib openssl libsystemd)
  "$work/userdata"
  printf 'Actual critical health passed:%s/sanitize%s; runtime fallback and power-loss tests remain separate.\n' "$compiler" "$sanitizer"
 done
done
