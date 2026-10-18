#!/usr/bin/env bash
# Run against the pinned UPower source after applying the supplied narrow patch.
set -Eeuo pipefail
source=${1:?Pass patched src/linux/up-device-supply-battery.c}
work=$(mktemp -d)
trap 'rm -r -- "$work"' EXIT
{
cat <<'C'
#include <glib.h>
#include <gio/gio.h>
#include <assert.h>
#include <string.h>
typedef unsigned UpDeviceSupplyBatteryChargeTypes;
enum { UP_DEVICE_SUPPLY_BATTERY_CHARGE_TYPES_STANDARD=16,
 UP_DEVICE_SUPPLY_BATTERY_CHARGE_TYPES_LONG_LIFE=128,
 UP_DEVICE_SUPPLY_BATTERY_CHARGE_TYPES_CUSTOM=64 };
typedef struct { gboolean charge_threshold_by_charge_type; } UpDevice;
typedef UpDevice UpDeviceSupplyBattery;
typedef UpDevice GUdevDevice;
#define UP_DEVICE_SUPPLY_BATTERY(d) (d)
#define G_UDEV_DEVICE(d) (d)
static void *up_device_get_native(UpDevice *d) { return d; }
static gboolean g_udev_device_has_sysfs_attr(GUdevDevice *d,const char *s) { (void)d; assert(!strcmp(s,"charge_types")); return TRUE; }
static gchar *up_device_supply_device_path(GUdevDevice *d) { (void)d; return g_strdup("/fixture"); }
static gchar *up_device_battery_charge_type_enum_to_str(unsigned type) { (void)type; return g_strdup("Custom"); }
static gboolean up_device_supply_battery_is_charge_type_exist(UpDevice *d,const char *type) { (void)d;(void)type; return TRUE; }
static unsigned up_device_battery_charge_find_available_charge_types_for_charging(UpDevice *d) { (void)d; return 16; }
static int calls, fail_at;
static char first_data[32], second_data[32];
static gboolean fake_write(const gchar *path,const gchar *data,gssize length,GFileSetContentsFlags flags,int mode,GError **error) {
 (void)path;(void)length;(void)flags;(void)mode; calls++;
 if(calls==1) g_strlcpy(first_data,data,sizeof first_data);
 if(calls==2) g_strlcpy(second_data,data,sizeof second_data);
 if(calls==fail_at) { g_set_error_literal(error,G_IO_ERROR,G_IO_ERROR_FAILED,"fixture write failure"); return FALSE; }
 return TRUE;
}
#define g_file_set_contents_full fake_write
C
awk '/^static gboolean$/ {hold=$0; next}
 /^up_device_supply_battery_set_battery_charge_types / || /^up_device_supply_battery_set_battery_charge_thresholds\(/ {emit=1;print hold}
 emit {print} emit && /^}/ {emit=0}' "$source"
cat <<'C'
int main(void) {
 UpDevice d={0}; GError *error=NULL;
 for(int failure=1;failure<=3;failure++) {
  calls=0;fail_at=failure;
  assert(!up_device_supply_battery_set_battery_charge_thresholds(&d,50,80,&error));
  assert(error && calls==failure); g_clear_error(&error);
 }
 calls=0;fail_at=0;
 assert(up_device_supply_battery_set_battery_charge_thresholds(&d,G_MAXUINT,80,&error) && calls==2 && !error);
 calls=0;
 assert(up_device_supply_battery_set_battery_charge_thresholds(&d,101,G_MAXUINT-1,&error) && calls==3);
 assert(!strcmp(first_data,"100") && !strcmp(second_data,"100"));
 calls=0;
 assert(up_device_supply_battery_set_battery_charge_thresholds(&d,0,100,&error) && calls==3);
 assert(!strcmp(first_data,"0") && !strcmp(second_data,"100"));
 calls=0;fail_at=1; d.charge_threshold_by_charge_type=TRUE;
 assert(!up_device_supply_battery_set_battery_charge_thresholds(&d,50,80,&error) && error);
 g_clear_error(&error); calls=0;fail_at=0;
 assert(up_device_supply_battery_set_battery_charge_thresholds(&d,0,100,&error) && calls==1);
 return 0;
}
C
} > "$work/threshold.c"
"${CC:-cc}" -std=gnu11 -Wall -Wextra -Werror "$work/threshold.c" $(pkg-config --cflags --libs glib-2.0 gio-2.0) -o "$work/threshold"
"$work/threshold"
echo 'Required UPower writes, charge_types errors and sentinel skipping passed; no installed-daemon or physical hold result'
