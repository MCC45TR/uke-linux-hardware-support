#!/usr/bin/env bash
set -Eeuo pipefail
test ! -e /etc/senemos/kernel-update.conf
here=$(cd -- "$(dirname -- "$0")/.." && pwd)
out=$(realpath -m "${1:?Build directory}")
mkdir -p "$out"
"$(pkg-config --variable=libexecdir Qt6Core)/moc" "$here/src/main.cpp" -o "$out/main.moc"
read -r -a qt_cflags <<< "$(pkg-config --cflags Qt6Core Qt6DBus)"
read -r -a qt_libs <<< "$(pkg-config --libs Qt6Core Qt6DBus)"
"${CXX:-g++}" ${CXXFLAGS:-} -std=c++20 -O2 -Wall -Wextra -Werror \
    "${qt_cflags[@]}" -I "$out" "$here/src/main.cpp" ${LDFLAGS:-} "${qt_libs[@]}" -o "$out/uke-pogo-desktop"
