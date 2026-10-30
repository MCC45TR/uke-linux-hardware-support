#!/usr/bin/env bash
# Link-time syscall injection into production updater helpers; no device access.
set -Eeuo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.."
mkdir -p build/tests
flags=(-std=c++20 -O1 -g -Wall -Wextra -Werror -Wno-unused-function)
if [[ ${SANITIZE:-0} == 1 ]]; then flags+=("-fsanitize=address,undefined" -fno-omit-frame-pointer); fi
read -ra dependencies <<< "$(pkg-config --cflags --libs fdisk zlib openssl libsystemd)"
"${CXX:-c++}" "${flags[@]}" tests/update-durability.cpp "${dependencies[@]}" \
    -Wl,--wrap=fsync,--wrap=pwrite,--wrap=renameat -o build/tests/update-durability
build/tests/update-durability
