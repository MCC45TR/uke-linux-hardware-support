#!/usr/bin/env bash
# Remote COPR fixtures only; no block device or target system is accessed.
set -Eeuo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.."
mkdir -p build/tests
flags=(-std=c++20 -O1 -g -UNDEBUG -Wall -Wextra -Werror -Wno-unused-function)
for variant in ordinary sanitized; do
    extra=()
    if [[ $variant == sanitized ]]; then
        extra=(-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer)
    fi
    "${CXX:-c++}" "${flags[@]}" "${extra[@]}" tests/development-trial.cpp \
        -o "build/tests/development-trial-$variant" $(pkg-config --cflags --libs fdisk zlib openssl libsystemd)
    ASAN_OPTIONS=detect_leaks=1:abort_on_error=1 UBSAN_OPTIONS=halt_on_error=1 "build/tests/development-trial-$variant"
done
