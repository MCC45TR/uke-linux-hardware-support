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
generator=/usr/lib/systemd/user-generators/systemd-xdg-autostart-generator
condition=/usr/lib/systemd/systemd-xdg-autostart-condition
test -x "$generator" && test -x "$condition"
mkdir -p "$work"/{config/autostart,empty,out,early,late}
# Substitute only the fixture executable; exercise the shipped desktop keys.
sed "s#^Exec=.*#Exec=$work/daemon#" "$root/src/uke-speaker-orientation.desktop" \
    > "$work/config/autostart/uke-speaker-orientation.desktop"
XDG_CONFIG_HOME="$work/config" XDG_CONFIG_DIRS="$work/empty" \
XDG_DATA_HOME="$work/empty" XDG_DATA_DIRS="$work/empty" SYSTEMD_SCOPE=user \
    "$generator" "$work/out" "$work/early" "$work/late"
mapfile -t units < <(find "$work/out" "$work/early" "$work/late" -type f -name '*.service')
[[ ${#units[@]} == 1 ]]
grep -Fq "ExecStart=:$work/daemon" "${units[0]}"
grep -Fq 'KDE:GNOME' "${units[0]}"
for desktop in KDE GNOME; do XDG_CURRENT_DESKTOP="$desktop" "$condition" 'KDE:GNOME' ''; done
if XDG_CURRENT_DESKTOP=fixture-other "$condition" 'KDE:GNOME' ''; then exit 1; fi
mkdir -p "$work/old/"{config/autostart,empty,out,early,late}
cp "$work/config/autostart/uke-speaker-orientation.desktop" "$work/old/config/autostart/"
printf 'X-GNOME-Autostart-Phase=Applications\n' >> "$work/old/config/autostart/uke-speaker-orientation.desktop"
XDG_CONFIG_HOME="$work/old/config" XDG_CONFIG_DIRS="$work/old/empty" \
XDG_DATA_HOME="$work/old/empty" XDG_DATA_DIRS="$work/old/empty" SYSTEMD_SCOPE=user \
    "$generator" "$work/old/out" "$work/old/early" "$work/old/late"
if grep -RFq 'KDE:GNOME' "$work/old/out" "$work/old/early" "$work/old/late"; then exit 1; fi
printf 'Actual systemd XDG generator and KDE/GNOME autostart conditions passed; no service started\n'
"${CXX:-c++}" "${flags[@]}" "${cflags[@]}" -fsanitize=address,undefined -fno-sanitize-recover=all \
    "$root/tests/speaker-orientation.cpp" "${libs[@]}" -o "$work/check"
"$work/check"
