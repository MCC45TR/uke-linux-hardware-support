#!/usr/bin/env bash
# Native UPower on fake sysfs and a private bus; no real power management.
set -Eeuo pipefail
if [[ ${1:-} != --inside ]]; then
    daemon=$(realpath "${1:?Pass the built UPower daemon}")
    work=$(mktemp -d)
    trap 'rm -r -- "$work"' EXIT
    cat > "$work/battery.umockdev" <<'EOF'
P: /devices/platform/late-capacity/power_supply/BAT0
E: SUBSYSTEM=power_supply
E: POWER_SUPPLY_NAME=BAT0
A: type=Battery
A: present=1
A: status=Full
A: capacity=82
A: voltage_min_design=4000000
A: voltage_now=4250000
A: current_now=100000
A: power_now=425000
A: charge_full_design=8850000
A: model_name=LateCapacityFixture
A: technology=Li-poly

P: /devices/platform/late-capacity/power_supply/AC
E: SUBSYSTEM=power_supply
E: POWER_SUPPLY_NAME=AC
A: type=Mains
A: online=1
EOF
    umockdev-run --device "$work/battery.umockdev" -- dbus-run-session -- bash "$0" --inside "$daemon"
    exit
fi
[[ -n ${UMOCKDEV_DIR:-} && -n ${DBUS_SESSION_BUS_ADDRESS:-} ]]
export DBUS_SYSTEM_BUS_ADDRESS=$DBUS_SESSION_BUS_ADDRESS
work=$(mktemp -d)
daemon_pid=''
cleanup() {
    result=$?
    if (( result != 0 )); then cat "$work/daemon.log" >&2; fi
    if [[ -n $daemon_pid ]]; then kill "$daemon_pid" 2>/dev/null || :; wait "$daemon_pid" 2>/dev/null || :; fi
    rm -r -- "$work"
}
trap cleanup EXIT
export UPOWER_CONF_FILE_NAME=$work/UPower.conf UPOWER_STATE_DIR=$work/state UPOWER_HISTORY_DIR=$work/history
mkdir "$UPOWER_STATE_DIR" "$UPOWER_HISTORY_DIR"
printf '[UPower]\nUsePercentageForPolicy=true\nPercentageLow=10\nPercentageCritical=5\nPercentageAction=2\n' > "$UPOWER_CONF_FILE_NAME"
"$2" -v -r > "$work/daemon.log" 2>&1 & daemon_pid=$!
battery=/org/freedesktop/UPower/devices/battery_BAT0
number() {
    busctl --address="$DBUS_SYSTEM_BUS_ADDRESS" --auto-start=no get-property org.freedesktop.UPower "$battery" org.freedesktop.UPower.Device "$1"
}
wait_number() {
    local property=$1 expected=$2
    for ((attempt=0; attempt<45; ++attempt)); do
        kill -0 "$daemon_pid" 2>/dev/null || return 1
        if number "$property" > "$work/value" 2>/dev/null &&
           awk -v expected="$expected" '$1=="d" && NF==2 {found=1;difference=$2-expected} END {exit !(found && difference<0.00001 && difference> -0.00001)}' "$work/value"; then return; fi
        sleep 1
    done
    printf 'UPower %s did not become %s\n' "$property" "$expected" >&2
    return 1
}
wait_number EnergyFull 0
fake_full=$UMOCKDEV_DIR/sys/devices/platform/late-capacity/power_supply/BAT0/charge_full
printf '8850000\n' > "$fake_full"
wait_number EnergyFull 35.4
wait_number EnergyRate 0.4
rm "$fake_full"
wait_number EnergyFull 0
wait_number EnergyRate 0.4
printf '8850000\n' > "$fake_full"
wait_number EnergyFull 35.4
wait_number EnergyRate 0.4
printf '%s\n' 'Actual UPower recovers late capacity and preserves charge units across unavailable reads; no physical result'
