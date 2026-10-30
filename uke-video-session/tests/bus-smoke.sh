#!/bin/bash
set -Eeuo pipefail
binary=$(realpath "${1:?service binary}")
work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT
chmod 700 "$work"
mkdir -p "$work/data/dbus-1/services" "$work/runtime"
chmod 700 "$work/runtime"
cat > "$work/data/dbus-1/services/org.uke.VideoSession1.service" <<SERVICE
[D-BUS Service]
Name=org.uke.VideoSession1
Exec=$binary
SERVICE
export XDG_DATA_HOME="$work/data" XDG_RUNTIME_DIR="$work/runtime"
export DBUS_SESSION_BUS_ADDRESS
# A private native D-Bus session tests activation and caller rejection only.
dbus-run-session -- bash -euo pipefail -c '
 busctl --user get-property org.uke.VideoSession1 /org/uke/VideoSession1 org.uke.VideoSession1 ActiveStreams | grep -x "u 0"
 if busctl --user call org.uke.VideoSession1 /org/uke/VideoSession1 org.uke.VideoSession1 Report tuub 1 3840 2160 true; then exit 1; fi
 busctl --user get-property org.uke.VideoSession1 /org/uke/VideoSession1 org.uke.VideoSession1 Held | grep -x "b false"
 read -r type pid < <(busctl --user call org.freedesktop.DBus /org/freedesktop/DBus org.freedesktop.DBus GetConnectionUnixProcessID s org.uke.VideoSession1)
 [[ $type == u && $pid =~ ^[0-9]+$ ]]
 kill -TERM "$pid"
'
