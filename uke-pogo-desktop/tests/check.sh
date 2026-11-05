#!/usr/bin/env bash
set -Eeuo pipefail
test ! -e /etc/senemos/kernel-update.conf
here=$(cd -- "$(dirname -- "$0")/.." && pwd)
stage=$(mktemp -d)
trap 'find "$stage" -depth -delete' EXIT
for compiler in g++ clang++; do
 "$compiler" -std=c++20 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
   -fno-sanitize-recover=all -fno-omit-frame-pointer "$here/tests/state.cpp" -o "$stage/check"
 "$stage/check"
done
