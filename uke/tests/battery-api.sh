#!/usr/bin/env bash
# Actual observer on a private bus. Host sysfs must not be a Uke device.
set -Eeuo pipefail
root=$(cd -- "$(dirname -- "$0")/.." && pwd)
if [[ ${1:-} != --inside ]]; then
    if [[ -r /sys/firmware/devicetree/base/compatible ]] && grep -azq 'xiaomi,uke' /sys/firmware/devicetree/base/compatible; then
        echo 'Host-only API fixture refuses a physical Uke' >&2; exit 77
    fi
    work=$(mktemp -d)
    trap 'rm -r -- "$work"' EXIT
    make -C "$root" outdir="$work/build" all
    dbus-run-session -- bash "$0" --inside "$work/build/uke-battery"
    exit
fi
[[ -n ${DBUS_SESSION_BUS_ADDRESS:-} ]]
export DBUS_SYSTEM_BUS_ADDRESS=$DBUS_SESSION_BUS_ADDRESS
"$2" --daemon & pid=$!
trap 'kill "$pid" 2>/dev/null || :; wait "$pid" 2>/dev/null || :' EXIT
ready=false
for ((i=0;i<100;i++)); do
    if caps=$(busctl --address="$DBUS_SYSTEM_BUS_ADDRESS" --auto-start=no --json=short call org.senemos.Uke.Battery1 /org/senemos/Uke/Battery1 org.senemos.Uke.Battery1 GetCapabilities 2>/dev/null); then ready=true; break; fi
    sleep 0.1
done
[[ $ready == true ]]
result=0
timeout -k 1s 5s "$2" --daemon >/dev/null 2>&1 || result=$?
[[ $result == 1 ]] # A second observer must not queue as another name owner.
jq -e '.data[0] | fromjson | .read_only and (.battery_setters | not) and (.charge_threshold | not) and (.hold | not) and (.high_power_control | not) and (.automatic_transport_recovery | not)' <<< "$caps" >/dev/null
for method in SetThreshold Hold SetCurrent SetRawProperty NegotiatePD ClearFault; do
    if busctl --address="$DBUS_SYSTEM_BUS_ADDRESS" --auto-start=no call org.senemos.Uke.Battery1 /org/senemos/Uke/Battery1 org.senemos.Uke.Battery1 "$method" >/dev/null 2>&1; then
        echo "Unexpected writable API: $method" >&2; exit 1
    fi
done
echo 'Actual private-bus observer exports no charging actuator or fault-clear API; no device test'
