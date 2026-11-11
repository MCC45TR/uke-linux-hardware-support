#!/usr/bin/bash
# Reversible software-only namespace fixture. Public data; no DSP or persist IO.
set -Eeuo pipefail
umask 077
export LC_ALL=C SYSTEMD_PAGER=cat SYSTEMD_COLORS=0
[[ $EUID == 0 && $# == 1 && -d /run/systemd/system ]]
output=$(realpath -m "$1")
[[ ! -e $output && ! -L $output ]]
mkdir -m700 "$output"
work=$(mktemp -d /run/uke-fastrpc-namespace.XXXXXX)
unit=$(basename "$work")
cleanup(){
 systemctl stop "$unit.service" >/dev/null 2>&1 || :
 systemctl reset-failed "$unit.service" >/dev/null 2>&1 || :
 find "$work" -depth -delete
}
trap cleanup EXIT
mkdir -p "$work"/{payload/registry,payload/config,payload/dsp,payload/socinfo,root/usr/bin,root/usr/lib64,root/usr/lib/qcom/adsp,root/vendor/etc/sensors/config,root/odm/etc/sensors/config,root/system,root/sys/devices/soc0,root/mnt/vendor/persist/sensors/registry/registry,root/persist/sensors/registry/registry}
for item in bin lib64;do ln -s "usr/$item" "$work/root/$item";done
ln -s usr/lib "$work/root/lib"
touch "$work/root/usr/lib/ld-linux-aarch64.so.1"
printf 'public registry vector\n' > "$work/payload/registry/public-vector"
printf 'public json list\n' > "$work/payload/config/json.lst"
printf 'public config vector\n' > "$work/payload/sns_reg.conf"
printf 'public firmware vector,not executable\n' > "$work/payload/dsp/sns_tppe.so"
printf 'public soc vector\n' > "$work/payload/socinfo/soc_id"
touch "$work/root/vendor/etc/sensors/sns_reg_config"
ln -s /vendor "$work/root/system/vendor"
sha256sum "$work/payload/config/json.lst" "$work/payload/sns_reg.conf" "$work/payload/dsp/sns_tppe.so" "$work/payload/socinfo/soc_id" > "$output/readonly-before.sha256"
systemd-run --quiet --wait --pipe --collect --unit="$unit" --setenv=LC_ALL=C \
 -p "RootDirectory=$work/root" \
 -p "BindReadOnlyPaths=/usr/bin /usr/lib64 /usr/lib/ld-linux-aarch64.so.1 $work/payload/config:/vendor/etc/sensors/config $work/payload/config:/odm/etc/sensors/config $work/payload/sns_reg.conf:/vendor/etc/sensors/sns_reg_config $work/payload/dsp:/usr/lib/qcom/adsp $work/payload/socinfo:/sys/devices/soc0" \
 -p "BindPaths=$work/payload/registry:/persist/sensors/registry/registry $work/payload/registry:/mnt/vendor/persist/sensors/registry/registry" \
 -p 'ReadWritePaths=/persist/sensors/registry/registry /mnt/vendor/persist/sensors/registry/registry' \
 -p ProtectSystem=strict -p ProtectHome=yes -p PrivateDevices=yes \
 -p TemporaryFileSystem=/sys:ro \
 -p MountAPIVFS=no \
 -p PrivateMounts=yes \
 -p NoNewPrivileges=yes -p CapabilityBoundingSet= \
 -p RestrictAddressFamilies=AF_UNIX -p TasksMax=8 -p MemoryMax=64M \
 -p RuntimeMaxSec=20 -p TimeoutStartSec=15 -p TimeoutStopSec=3 \
 -- /usr/bin/bash --noprofile --norc -ceu '
  test ! -e /dev/fastrpc-adsp
  test ! -e /sys/class/remoteproc
  grep -Fqx "public soc vector" /sys/devices/soc0/soc_id
  test ! -e /etc/senemos/kernel-update.conf
  cmp /persist/sensors/registry/registry/public-vector /mnt/vendor/persist/sensors/registry/registry/public-vector
  cmp /vendor/etc/sensors/config/json.lst /odm/etc/sensors/config/json.lst
  cmp /vendor/etc/sensors/sns_reg_config /system/vendor/etc/sensors/sns_reg_config
  grep -Fqx "public firmware vector,not executable" /usr/lib/qcom/adsp/sns_tppe.so
  printf "public changed registry\n" > /persist/sensors/registry/registry/new-vector
  grep -Fqx "public changed registry" /mnt/vendor/persist/sensors/registry/registry/new-vector
  if (printf bad >> /vendor/etc/sensors/config/json.lst) 2>/dev/null; then exit 1; fi
  if (printf bad >> /vendor/etc/sensors/sns_reg_config) 2>/dev/null; then exit 1; fi
  if (printf bad >> /usr/lib/qcom/adsp/sns_tppe.so) 2>/dev/null; then exit 1; fi
  if (printf bad >> /sys/devices/soc0/soc_id) 2>/dev/null; then exit 1; fi
  printf "Native RootDirectory/binds:shared synthetic registry,readonly config/firmware,aliases and absent DSP/host settings passed.\n"
 ' < /dev/null > "$output/result.log" 2>&1
sha256sum -c --strict --quiet "$output/readonly-before.sha256"
grep -Fqx 'public changed registry' "$work/payload/registry/new-vector"
cleanup
trap - EXIT
[[ ! -e $work ]]
printf 'Only public fixture data and a collected transient unit were used;own temporary tree removed.\n' >> "$output/result.log"
touch "$output/complete"
cat "$output/result.log"

