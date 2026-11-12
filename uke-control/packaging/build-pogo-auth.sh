#!/usr/bin/bash
# COPR only. Original MINK adapter, linked to the original Fedora QCOMTEE.
set -Eeuo pipefail
[[ ! -e /etc/senemos/kernel-update.conf && $(uname -m) == aarch64 ]]
root=$PWD
up=$root/upstream/minkipc
mkdir -p "$root/build/objects"
objects=()
for source in "$up/libminkadaptor/src/mink_adaptor.c" \
 "$up/libminkadaptor/src/supplicant.c" "$up/libminkadaptor/src/syscall.S";do
 object=$root/build/objects/${source##*/}.o
 ${CC:-gcc} ${CFLAGS:-} -std=gnu11 -D_GNU_SOURCE -fPIC -pthread \
  -I"$up/libminkadaptor/include" -I"$up/libminkadaptor/src" $(pkg-config --cflags qcomtee) \
  -c "$source" -o "$object"
 objects+=("$object")
done
${CXX:-g++} ${CXXFLAGS:-} -std=c++20 -Wall -Wextra -Werror -Wno-misleading-indentation \
 -I"$up/libminkadaptor/include" $(pkg-config --cflags libsystemd) \
 "$root/uke-pogo-auth/src/main.cpp" "${objects[@]}" ${LDFLAGS:-} \
 -Wl,--no-undefined,-z,noexecstack -pthread -lcrypto \
 $(pkg-config --libs libsystemd qcomtee) -o "$root/build/uke-pogo-auth"
