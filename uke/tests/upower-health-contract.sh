#!/usr/bin/env bash
# Actual patched UPower helper; unknown capacity is 0 in this D-Bus API.
set -Eeuo pipefail
source=${1:?Pass patched src/up-device-battery.c}
work=$(mktemp -d)
trap 'rm -r -- "$work"' EXIT
{
printf '%s\n' '#include <glib.h>' '#include <assert.h>'
awk '/^#include </ {print}' "$source"
awk '/^static gdouble$/ {hold=$0;next}
 /^up_device_battery_calc_capacity / {emit=1;print hold}
 emit {print} emit && /^}/ {exit}' "$source"
cat <<'C'
int main(void) {
 assert(up_device_battery_calc_capacity(0,40)==0);
 assert(up_device_battery_calc_capacity(30,0)==0);
 assert(up_device_battery_calc_capacity(NAN,40)==0);
 assert(up_device_battery_calc_capacity(30,INFINITY)==0);
 assert(fabs(up_device_battery_calc_capacity(36,40)-90)<0.001);
 assert(up_device_battery_calc_capacity(44,40)==100);
 return 0;
}
C
} > "$work/health.c"
"${CC:-cc}" -std=gnu11 -Wall -Wextra -Werror "$work/health.c" $(pkg-config --cflags --libs glib-2.0) -lm -o "$work/health"
"$work/health"
echo 'UPower capacity arithmetic passed; kernel/gauge learned-capacity semantics and installed-daemon acceptance remain open'
