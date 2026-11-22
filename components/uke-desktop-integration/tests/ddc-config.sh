#!/bin/sh
# Offline topology and configuration precedence; no physical I2C access.
set -eu
project=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT
sysfs=$work/sys
mkdir -p "$sysfs/firmware/devicetree/base" "$sysfs/bus/platform/drivers/geni_i2c" \
    "$sysfs/bus/platform/drivers/msm-dp-display" "$work/run" "$work/home" "$work/etc" "$work/vendor"
printf 'xiaomi,uke\000qcom,sm7675\000' > "$sysfs/firmware/devicetree/base/compatible"
for number in 2 5 6 300; do
    parent=$sysfs/devices/platform/controller-$number
    mkdir -p "$parent/i2c-$number" "$sysfs/class/i2c-dev/i2c-$number"
    if test "$number" = 6; then driver=msm-dp-display; else driver=geni_i2c; fi
    ln -s "$sysfs/bus/platform/drivers/$driver" "$parent/driver"
    ln -s "$parent/i2c-$number" "$sysfs/class/i2c-dev/i2c-$number/device"
done
export XDG_RUNTIME_DIR=$work/run XDG_CONFIG_HOME=$work/home/.config
export XDG_CONFIG_DIRS=$work/etc:$work/vendor
run_helper() { sh "$project/src/uke-ddc-config" "$sysfs"; }
run_helper
config=$work/run/uke-display-ddc/ddcutil/ddcutilrc
grep -Fxq 'options = --disable-early-permission-checks --ignore-bus 2 --ignore-bus 5' "$config"
if grep -Fq -- '--ignore-bus 300' "$config"; then exit 1; fi
test "$(stat -c %a "$config")" = 600
test "$(stat -c %a "$work/run/uke-display-ddc.env")" = 600
expected="XDG_CONFIG_DIRS=\"$work/run/uke-display-ddc:$work/etc:$work/vendor\""
grep -Fxq "$expected" "$work/run/uke-display-ddc.env"
export XDG_CONFIG_DIRS=$work/run/uke-display-ddc:$work/etc:$work/vendor
run_helper
grep -Fxq "$expected" "$work/run/uke-display-ddc.env"
mkdir -p "$work/etc/ddcutil"
printf 'administrator configuration\n' > "$work/etc/ddcutil/ddcutilrc"
run_helper
test ! -e "$work/run/uke-display-ddc.env"
grep -Fxq 'administrator configuration' "$work/etc/ddcutil/ddcutilrc"
rm "$work/etc/ddcutil/ddcutilrc"
mkdir -p "$XDG_CONFIG_HOME/ddcutil"
printf 'user configuration\n' > "$XDG_CONFIG_HOME/ddcutil/ddcutilrc"
run_helper
test ! -e "$work/run/uke-display-ddc.env"
grep -Fxq 'user configuration' "$XDG_CONFIG_HOME/ddcutil/ddcutilrc"
rm "$XDG_CONFIG_HOME/ddcutil/ddcutilrc"
rm -rf "$work/run/uke-display-ddc"
printf 'other,board\000' > "$sysfs/firmware/devicetree/base/compatible"
run_helper
test ! -e "$work/run/uke-display-ddc"
printf 'GENI exclusion, DP retention, private files, restart and custom-config precedence: passed\n'
