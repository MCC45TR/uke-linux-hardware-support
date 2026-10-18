#!/usr/bin/env bash
# Deferred host fixture; uses the actual read-only observer, never device sysfs.
set -Eeuo pipefail
here=$(cd -- "$(dirname -- "$0")/.." && pwd)
work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT
c++ -std=c++17 -Wall -Wextra -Werror "$here/src/usb-port.cpp" -o "$work/observer" \
    $(pkg-config --cflags --libs libsystemd libdrm)
mkdir -p "$work/root/sys/firmware/devicetree/base" "$work/root/proc/sys/kernel" \
    "$work/root/sys/class/typec/port7" "$work/root/sys/class/udc/controller-renamed"
printf 'xiaomi,uke\0qcom,sm7675\0' > "$work/root/sys/firmware/devicetree/base/compatible"
printf '7.2.10-7\n' > "$work/root/proc/sys/kernel/osrelease"
printf 'host\n' > "$work/root/sys/class/typec/port7/data_role"
printf 'sink\n' > "$work/root/sys/class/typec/port7/power_role"
printf 'unknown\n' > "$work/root/sys/class/udc/controller-renamed/state"
"$work/observer" --root "$work/root" > "$work/result.json"
jq -e '.target_identity_matches and .control_authorized == false and
 .coherence == "unproven_sequential_reads" and .attach_generation == null and
 .classes.typec[0].attributes.data_role == "host" and
 .classes.typec[0].attributes.power_role == "sink" and
 .classes.typec[0].attributes.orientation == null and
 .classes.udc[0].class_name == "controller-renamed" and
 .trained_dp_lanes == null and .physical_acceptance == "NOT_RUN"' "$work/result.json"
