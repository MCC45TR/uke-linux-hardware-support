#!/usr/bin/env bash
# Synthetic sysfs only. Does not activate UPower, D-Bus or a hardware provider.
set -Eeuo pipefail
root=$(cd -- "$(dirname -- "$0")/.." && pwd)
work=$(mktemp -d)
trap 'rm -r -- "$work"' EXIT
make -C "$root" outdir="$work/build" all
sys=$work/sys
mkdir -p "$sys/firmware/devicetree/base" "$sys/class/power_supply" "$sys/devices/charger" "$sys/bus/auxiliary/drivers/pmic_glink_power_supply"
printf 'xiaomi,uke\0qcom,sm7675\0' > "$sys/firmware/devicetree/base/compatible"
ln -s "$sys/bus/auxiliary/drivers/pmic_glink_power_supply" "$sys/devices/charger/driver"
for name in bat usb; do
    supply=$sys/class/power_supply/qcom-battmgr-$name
    mkdir "$supply"
    ln -s "$sys/devices/charger" "$supply/device"
done
printf '1 1 1 1 1 0 0 0 0 0 100 99\n' > "$sys/devices/charger/uke_battmgr_state"
printf 'Discharging\n' > "$sys/class/power_supply/qcom-battmgr-bat/status"
printf 'Good\n' > "$sys/class/power_supply/qcom-battmgr-bat/health"
printf '%s\n' -500000 > "$sys/class/power_supply/qcom-battmgr-bat/current_now"
printf '%s\n' -2000000 > "$sys/class/power_supply/qcom-battmgr-bat/power_now"
printf '0\n' > "$sys/class/power_supply/qcom-battmgr-bat/cycle_count"
printf '1\n' > "$sys/class/power_supply/qcom-battmgr-usb/online"
mkdir -p "$sys/class/thermal/thermal_zone0"
printf 'pmic_bcl_current\n' > "$sys/class/thermal/thermal_zone0/type"
printf '125000\n' > "$sys/class/thermal/thermal_zone0/temp"
"$work/build/uke-battery" --sysfs-root "$sys" > "$work/snapshot.json"
jq -e '.origin == "fixture" and .provider_verified and .snapshot_coherent and .external_power_with_negative_battery_current and .fields["battery.power_now"].value == -2000000 and .fields["battery.cycle_count"].value == null and .health.capacity_retention_percent == null and .forecast.time_to_full_seconds == null and .forecast.reason == "profile_unqualified" and .efc_coverage.checkpoint == "disabled_unqualified" and .capabilities.read_only and (.capabilities.charge_threshold | not)' "$work/snapshot.json" >/dev/null
jq -e '.fields["battery.temp"].value == null and .safety == "unknown_safety_state"' "$work/snapshot.json" >/dev/null
printf '1 2 1 2 1 1 0 0 -110 0 100 99\n' > "$sys/devices/charger/uke_battmgr_state"
"$work/build/uke-battery" --sysfs-root "$sys" > "$work/stale.json"
jq -e '(.snapshot_coherent | not) and .status == "transport_quiesced" and .recovery == "cold_boot_required" and .fields == {} and .physical_charge_stopped == null' "$work/stale.json" >/dev/null
printf 'xiaomi,other\0' > "$sys/firmware/devicetree/base/compatible"
"$work/build/uke-battery" --sysfs-root "$sys" --inventory > "$work/other.json"
jq -e '(.provider_verified | not) and .status == "provider_unverified" and (.fields == null)' "$work/other.json" >/dev/null
if "$work/build/uke-battery" --watch 0 >/dev/null 2>&1; then exit 1; fi
if "$work/build/uke-battery" --interval-seconds 0 >/dev/null 2>&1; then exit 1; fi
if "$work/build/uke-battery" --interval-seconds 61 >/dev/null 2>&1; then exit 1; fi
echo 'Synthetic collector provider, signed power, missing data and stale snapshot checks passed; no Uke device result'
