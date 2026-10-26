#!/usr/bin/env bash
# Exercise the real RPM reload block with an isolated socket and command stubs.
set -Eeuo pipefail
root=$(cd "$(dirname "$0")/.." && pwd)
stage=$(mktemp -d)
bus_pid=''
trap '[[ -z $bus_pid ]] || kill "$bus_pid"; find "$stage" -depth -delete' EXIT
awk '/^if .*systemctl -q is-active dbus.service; then$/ {copy=1} copy {print} copy && /^fi$/ {exit}' \
    "$root/packaging/uke-control.spec" | sed \
    -e "s|/run/dbus/system_bus_socket|$stage/bus|g" \
    -e "s|/usr/bin/systemctl|$stage/systemctl|g" \
    -e "s|/usr/bin/busctl|$stage/busctl|g" > "$stage/reload.sh"
test -s "$stage/reload.sh"
cat > "$stage/systemctl" <<'SH'
#!/bin/sh
exit "${DBUS_ACTIVE:-0}"
SH
cat > "$stage/busctl" <<'SH'
#!/bin/sh
printf 'reload\n' >> "$RELOAD_LOG"
exit "${RELOAD_EXIT:-0}"
SH
chmod 755 "$stage/systemctl" "$stage/busctl"
export RELOAD_LOG=$stage/reloads
: > "$RELOAD_LOG"
# systemctl may report success when ignoring an is-active query in a chroot.
DBUS_ACTIVE=0 bash "$stage/reload.sh"
test ! -s "$RELOAD_LOG"
bus_pid=$(dbus-daemon --session --address="unix:path=$stage/bus" --fork --print-pid)
DBUS_ACTIVE=3 bash "$stage/reload.sh"
test ! -s "$RELOAD_LOG"
DBUS_ACTIVE=0 bash "$stage/reload.sh"
test "$(cat "$RELOAD_LOG")" = reload
if DBUS_ACTIVE=0 RELOAD_EXIT=1 bash "$stage/reload.sh"; then exit 1; fi
printf '%s\n' 'Offline reload refusal, inactive/live bus and reload-error propagation passed'
