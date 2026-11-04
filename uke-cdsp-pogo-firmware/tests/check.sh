#!/usr/bin/env bash
# COPR-only native fixtures; no firmware or tablet partition access.
set -Eeuo pipefail
here=$(cd -- "$(dirname -- "$0")/.." && pwd)
work=$(mktemp -d)
trap 'rm -r -- "$work"' EXIT
for source in contract idle; do
"${CXX:-c++}" -std=c++20 -O1 -g -Wall -Wextra -Werror -Wno-misleading-indentation \
    -fsanitize=address,undefined -fno-omit-frame-pointer \
    "$here/tests/$source.cpp" -lcrypto -o "$work/check"
"$work/check"
done
