#!/bin/bash
set -euo pipefail
component=$(cd -- "$(dirname -- "$0")/.." && pwd)
fixture=$(mktemp -d)
trap 'rm -rf -- "$fixture"' EXIT
mkdir -p "$fixture"/{mnt/vendor/persist/sensors/registry/registry,usr/share/uke-sensors/odm,usr/lib/firmware/qcom/uke/sensors,sys/devices/soc0,run/uke-ssc}
printf 'calibration fixture\n' > "$fixture/mnt/vendor/persist/sensors/registry/registry/calibration"
printf 'config fixture\n' > "$fixture/usr/share/uke-sensors/odm/json.lst"
printf 'version=14\n' > "$fixture/usr/share/uke-sensors/sns_reg.conf"
printf 'DSP fixture; not executable\n' > "$fixture/usr/lib/firmware/qcom/uke/sensors/sns_tppe.so"
for name in hw_platform platform_subtype platform_subtype_id platform_version soc_id revision; do
    printf 'fixture\n' > "$fixture/sys/devices/soc0/$name"
done
original=$(sha256sum "$fixture/mnt/vendor/persist/sensors/registry/registry/calibration")
systemd-tmpfiles --root="$fixture" --create "$component/packaging/uke-ssc.conf"
cmp "$fixture/mnt/vendor/persist/sensors/registry/registry/calibration" "$fixture/run/uke-ssc/sensors/persist/registry/calibration"
cmp "$fixture/usr/share/uke-sensors/odm/json.lst" "$fixture/run/uke-ssc/sensors/config/json.lst"
test "$(stat -c %a "$fixture/run/uke-ssc/sensors")" = 700
printf 'RAM-only update\n' > "$fixture/run/uke-ssc/sensors/persist/registry/calibration"
systemd-tmpfiles --root="$fixture" --create "$component/packaging/uke-ssc.conf"
grep -qx 'RAM-only update' "$fixture/run/uke-ssc/sensors/persist/registry/calibration"
test "$(sha256sum "$fixture/mnt/vendor/persist/sensors/registry/registry/calibration")" = "$original"
printf 'Calibration copy, private modes and RAM-only restart state passed (fixture only)\n'
