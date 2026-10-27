#!/usr/bin/env bash
set -Eeuo pipefail
root=$(cd -- "$(dirname -- "$0")/.." && pwd)
work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT
read -r -a cflags <<< "$(pkg-config --cflags alsa wayland-client libsystemd)"
read -r -a libs <<< "$(pkg-config --libs alsa wayland-client libsystemd)"
flags=(-std=c++17 -O1 -g -Wall -Wextra -Werror)
"${CXX:-c++}" "${flags[@]}" "${cflags[@]}" "$root/src/uke-speaker-orientation.cpp" "${libs[@]}" -o "$work/daemon"
XDG_SESSION_TYPE=x11 "$work/daemon"
"${CXX:-c++}" "${flags[@]}" "${cflags[@]}" -fsanitize=address,undefined -fno-sanitize-recover=all \
    "$root/tests/speaker-orientation.cpp" "${libs[@]}" -o "$work/check"
"$work/check"
