#!/usr/bin/bash
# SPDX-License-Identifier: MIT
set -Eeuo pipefail
[[ ${1:-} == --execute && $# == 1 ]] || {
    echo 'Usage: uke-fidelityfx-gpu-test --execute' >&2
    exit 2
}
unit=uke-fidelityfx-gpu-test.service
gpu_zone=
for zone in /sys/class/thermal/thermal_zone*; do
    [[ $(cat "$zone/type") != gpuss-0 ]] || { gpu_zone=$zone; break; }
done
[[ -n $gpu_zone ]] || { echo 'GPU thermal sensor unavailable' >&2; exit 1; }
available=$(awk '/MemAvailable/ {print $2}' /proc/meminfo)
(( available >= 2097152 && $(cat "$gpu_zone/temp") < 65000 )) || {
    echo 'Refusing test: require 2 GiB available memory and GPU below 65 C' >&2
    exit 1
}
! systemctl --user is-active --quiet "$unit" || { echo 'Test already active' >&2; exit 1; }
icd=/usr/share/vulkan/icd.d/freedreno_icd.aarch64.json
[[ -r $icd ]] || { echo 'Fedora Turnip ICD unavailable' >&2; exit 1; }
record=$(mktemp -d "${XDG_CACHE_HOME:-$HOME/.cache}/uke-fidelityfx.XXXXXXXX")
chmod 700 "$record"
trap 'systemctl --user stop "$unit" >/dev/null 2>&1 || true' EXIT
printf 'Private evidence: %s\n' "$record"
systemd-run --user --unit="$unit" --collect \
    --property=RuntimeMaxSec=45 --property=TimeoutStopSec=2 \
    --property=MemoryMax=1536M --property=KillMode=control-group \
    --property="StandardOutput=append:$record/gpu.log" \
    --property="StandardError=append:$record/gpu.log" \
    --working-directory="$record" --setenv=LD_BIND_NOW=1 \
    --setenv="VK_DRIVER_FILES=$icd" --setenv="MESA_SHADER_CACHE_DIR=$record/cache" \
    /bin/sh -c '/usr/bin/uke-fidelityfx-gpu-check --execute; status=$?; printf "%s\n" "$status" > exit-code; exit "$status"'
for ((sample=0; sample<25; ++sample)); do
    temperature=$(cat "$gpu_zone/temp")
    available=$(awk '/MemAvailable/ {print $2}' /proc/meminfo)
    printf '%s %s %s\n' "$sample" "$temperature" "$available" >> "$record/temperature-memory.txt"
    if (( temperature >= 75000 || available < 524288 )); then
        echo "STOP: GPU temperature=$temperature memory=$available" >&2
        exit 97
    fi
    [[ ! -f $record/exit-code ]] || break
    systemctl --user is-active --quiet "$unit" || break
    sleep 2
done
cat "$record/gpu.log"
[[ -s $record/exit-code && $(cat "$record/exit-code") == 0 ]]
