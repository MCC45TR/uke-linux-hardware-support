#!/usr/bin/env bash
# Native synthetic fixtures run on COPR, with no TEE or tablet access.
set -Eeuo pipefail
here=$(cd -- "$(dirname -- "$0")/.." && pwd)
mink=${1:?Pass original MINK source directory}
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
"${CXX:-g++}" -std=c++20 -O1 -g -Wall -Wextra -Werror -Wno-misleading-indentation \
    -fsanitize=address,undefined -fno-omit-frame-pointer \
    -I"$mink/libminkadaptor/include" "$here/tests/contract.cpp" -lcrypto -o "$work/check"
"$work/check"
