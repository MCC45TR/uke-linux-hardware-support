#!/usr/bin/env bash
# COPR only. Include the complete actual parent C file, using real GLib tasks.
set -Eeuo pipefail
source_file=$(realpath "${1:?prepared libssc-sensor.c}")
seam=${2:?C seam}
driver=${3:?C++ driver}
work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT
mapfile -t dirs < <(find build/src -maxdepth 1 -type d -name 'libssc.so*.p')
[[ ${#dirs[@]} == 1 ]]
mapfile -t objects < <(find "${dirs[0]}" -type f -name '*.o' ! -name '*libssc-sensor.c.o' | sort)
(( ${#objects[@]} >= 8 ))
flags=(-UNDEBUG -O1 -g -Wall -Wextra -Werror -Wno-unused-parameter -Wno-cast-function-type)
if [[ ${SANITIZE:-0} == 1 ]]; then
 flags+=(-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer)
fi
read -ra cflags <<< "$(pkg-config --cflags gio-2.0 qmi-glib libprotobuf-c)"
read -ra libs <<< "$(pkg-config --libs gio-2.0 qmi-glib libprotobuf-c)"
cc -std=gnu11 "${flags[@]}" "${cflags[@]}" -I src -I build/src -I data -I build/data \
 "-DLIFECYCLE_SOURCE=\"$source_file\"" -c "$seam" -o "$work/parent.o"
c++ -std=c++20 "${flags[@]}" "${cflags[@]}" -I src -I build/src -I data -I build/data \
 "$driver" "$work/parent.o" "${objects[@]}" "${libs[@]}" -lm -o "$work/lifecycle"
G_DEBUG=fatal-criticals G_SLICE=always-malloc "$work/lifecycle"
