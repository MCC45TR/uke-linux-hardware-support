#!/usr/bin/bash
# COPR native software qualification only: no tablet, firmware or TEE access.
set -Eeuo pipefail
[[ ! -e /etc/senemos/kernel-update.conf && $# == 1 ]]
root=$(realpath "$1")
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
ulimit -c 0
for compiler in g++ clang++;do
 "$compiler" -std=c++20 -O1 -g -Wall -Wextra -Werror -pthread \
  -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer \
  "$root/tests/pogo-runtime.cpp" -o "$work/pogo-runtime"
 timeout --signal=TERM --kill-after=2s 20s "$work/pogo-runtime"
done
printf 'Pogo runtime: admitted families, stop-before-poll, blocked stop, request/removal and handler restoration pass.\n'
