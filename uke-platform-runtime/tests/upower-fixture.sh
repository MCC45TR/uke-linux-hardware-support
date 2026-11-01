#!/usr/bin/env bash
# Isolated host fixture. No real system bus, sysfs writes or tablet access.
set -Eeuo pipefail
root=$(cd -- "$(dirname -- "$0")/.." && pwd)
if [[ ${1:-} != --inside ]]; then
    daemon=$(realpath "${1:?Pass the pinned, reviewed UPower build's upowerd}")
    exec umockdev-run --device "$root/tests/fixtures/upower/ac-net-discharge.umockdev" -- \
        dbus-run-session -- bash "$0" --inside "$daemon"
fi
[[ -n ${UMOCKDEV_DIR:-} && -n ${DBUS_SESSION_BUS_ADDRESS:-} ]]
[[ -d $UMOCKDEV_DIR && $UMOCKDEV_DIR != / ]]
export DBUS_SYSTEM_BUS_ADDRESS=$DBUS_SESSION_BUS_ADDRESS
export SYSTEMD_DEVICE_VERIFY_SYSFS=0
work=$(mktemp -d)
daemon_pid=
cleanup() {
    if [[ -n $daemon_pid ]]; then kill "$daemon_pid" 2>/dev/null || :; wait "$daemon_pid" 2>/dev/null || :; fi
    rm -r -- "$work"
}
trap cleanup EXIT
export UPOWER_CONF_FILE_NAME=$work/UPower.conf UPOWER_STATE_DIR=$work/state UPOWER_HISTORY_DIR=$work/history
mkdir "$UPOWER_STATE_DIR" "$UPOWER_HISTORY_DIR"
printf '[UPower]\nUsePercentageForPolicy=true\nPercentageLow=10\nPercentageCritical=5\nPercentageAction=2\n' > "$UPOWER_CONF_FILE_NAME"
"$2" -v -r > "$work/daemon.log" 2>&1 & daemon_pid=$!
ready=false
for ((attempt=0; attempt<100; ++attempt)); do
    if busctl --address="$DBUS_SYSTEM_BUS_ADDRESS" --auto-start=no get-property org.freedesktop.UPower /org/freedesktop/UPower org.freedesktop.UPower OnBattery > "$work/on-battery" 2>/dev/null && grep -Fxq "b true" "$work/on-battery"; then ready=true; break; fi
    sleep 0.1
done
[[ $ready == true ]]
grep -Fx 'b true' "$work/on-battery"
display=/org/freedesktop/UPower/devices/DisplayDevice
busctl --address="$DBUS_SYSTEM_BUS_ADDRESS" --auto-start=no get-property org.freedesktop.UPower "$display" org.freedesktop.UPower.Device State > "$work/display-state"
grep -Fx 'u 2' "$work/display-state"
warning=$(busctl --address="$DBUS_SYSTEM_BUS_ADDRESS" --auto-start=no get-property org.freedesktop.UPower "$display" org.freedesktop.UPower.Device WarningLevel)
[[ $warning == 'u 3' || $warning == 'u 4' || $warning == 'u 5' ]]
printf '%s\n' 'Isolated UPower AC-online/discharging/low-battery state passed; real desktop actions and physical charging remain untested'
