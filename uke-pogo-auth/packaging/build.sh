#!/usr/bin/env bash
# COPR only. Compile fixed original sources without upstream build scripts/idlc.
set -Eeuo pipefail
test ! -e /etc/senemos/kernel-update.conf
[[ $(uname -m) == aarch64 ]]
here=$(cd -- "$(dirname -- "$0")/.." && pwd)
root=$(cd -- "$here/.." && pwd)
up=$root/upstream
mkdir -p "$root/build/objects"
objects=()
includes=(-I"$up/quic-teec/libqcomtee/include" -I"$up/quic-teec/libqcomtee/src"
          -I"$up/minkipc/libminkadaptor/include" -I"$up/minkipc/libminkadaptor/src" -I"$up/qcbor/inc")
for source in "$up"/qcbor/src/*.c \
    "$up/quic-teec/libqcomtee/src/qcomtee_object.c" \
    "$up/quic-teec/libqcomtee/src/objects/credentials_obj.c" \
    "$up/quic-teec/libqcomtee/src/objects/mem_obj.c" \
    "$up/minkipc/libminkadaptor/src/mink_adaptor.c" \
    "$up/minkipc/libminkadaptor/src/supplicant.c" \
    "$up/minkipc/libminkadaptor/src/syscall.S"; do
    object=$root/build/objects/${source##*/}.o
    # Fedora compiler flags contain intentional shell-separated arguments.
    ${CC:-gcc} ${CFLAGS:-} -std=gnu11 -D_GNU_SOURCE -DUSE_QCBOR -fPIC -pthread \
        "${includes[@]}" -c "$source" -o "$object"
    objects+=("$object")
done
${CXX:-g++} ${CXXFLAGS:-} -std=c++20 -Wall -Wextra -Werror -Wno-misleading-indentation \
    "${includes[@]}" $(pkg-config --cflags libsystemd) "$here/src/main.cpp" "${objects[@]}" ${LDFLAGS:-} \
    -Wl,--no-undefined,-z,noexecstack -pthread -lcrypto -lm $(pkg-config --libs libsystemd) -o "$root/build/uke-pogo-auth"
