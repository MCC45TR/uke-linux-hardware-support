#!/usr/bin/env bash
# COPR only: actual patched upstream C, real QCBOR and injected Finish failures.
set -Eeuo pipefail
test ! -e /etc/senemos/kernel-update.conf
here=$(cd -- "$(dirname -- "$0")/.." && pwd)
up=${1:?Pass the extracted upstream directory}
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
"${CC:-gcc}" -std=gnu11 -D_GNU_SOURCE -O1 -g -Wall -Wextra -Werror \
    -fsanitize=address,undefined -fno-omit-frame-pointer \
    -ffunction-sections -fdata-sections -Wl,--gc-sections \
    -I"$up/quic-teec/libqcomtee/include" -I"$up/quic-teec/libqcomtee/src" -I"$up/qcbor/inc" \
    "$here/tests/credentials.c" "$up"/qcbor/src/*.c -lm -pthread -o "$work/credentials"
"$work/credentials"
