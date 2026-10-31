#!/bin/bash
# SPDX-License-Identifier: MIT
# Optional OEM references; never write an OEM block device or replace RPM files.
set -Eeuo pipefail
export LC_ALL=C PATH=/usr/sbin:/usr/bin
umask 077
base=/run/uke-oem-resources
lib=/usr/libexec/uke-oem-resources
fail() { printf '%s\n' "$*" >&2; exit 1; }
u32() { od --endian="$1" -An -tu4 -j "$3" -N4 "$2" | tr -d '[:space:]'; }
read_region() {
 local source=$1 offset=$2 length=$3 output=$4 capacity
 capacity=$(blockdev --getsize64 "$source")
 ((offset>=0 && length>0 && length<=capacity && offset<=capacity-length)) || fail region_bounds
 dd if="$source" of="$output" bs=64K skip="$offset" count="$length" iflag=skip_bytes,count_bytes,fullblock status=none
 test "$(stat -c %s "$output")" = "$length" || fail short_read
}
partition() {
 local label=$1 path
 case $label in modem_[ab]|dsp_[ab]|bluetooth_[ab]|dtbo_[ab]|super) ;; *) fail unapproved_partition;; esac
 path=$(readlink -e "/dev/disk/by-partlabel/$label")
 test -b "$path" || fail missing_partition
 printf '%s\n' "$path"
}
mount_region() {
 local name=$1 source=$2 offset=$3 length=$4 type=$5 loop options
 [[ $name =~ ^[a-z0-9_]+$ ]] || fail mount_name
 test ! -e "$base/mounts/$name" || fail existing_mount_target
 mkdir "$base/mounts/$name"
 loop=$(losetup --find --show --read-only --offset "$offset" --sizelimit "$length" "$source")
 printf '%s\t%s\t%s\t%s\t%s\n' "$loop" "$source" "$offset" "$length" "$name" >> "$base/loops.tsv"
 test "$(blockdev --getro "$loop")" = 1 || fail writable_loop
 test "$(blkid -p -s TYPE -o value "$loop")" = "$type" || fail filesystem_type
 case $type in ext4) options=ro,noload,nosuid,nodev,noexec;; vfat) options=ro,nosuid,nodev,noexec,umask=0077;; *) fail unsupported_filesystem;; esac
 # A mount-view context avoids modifying Android SELinux extended attributes.
 options+=,context=system_u:object_r:lib_t:s0
 mount -t "$type" -o "$options" "$loop" "$base/mounts/$name"
 findmnt --mountpoint "$base/mounts/$name" --options ro --noheadings >/dev/null || fail writable_mount
}
stop() {
 local loop source offset length name current
 if test -f "$base/loops.tsv"; then
  while IFS=$'\t' read -r loop source offset length name; do
   [[ $loop =~ ^/dev/loop[0-9]+$ && $source =~ ^/dev/[a-zA-Z0-9]+$ && $name =~ ^[a-z0-9_]+$ && $offset =~ ^[0-9]+$ && $length =~ ^[0-9]+$ ]] || fail invalid_owned_mapping
   if losetup --list --json "$loop" | jq -e --arg file "$source" --argjson offset "$offset" --argjson length "$length" '.loopdevices | length==1 and .[0]["back-file"]==$file and .[0].offset==$offset and .[0].sizelimit==$length and .[0].ro==true' >/dev/null; then
    if findmnt --mountpoint "$base/mounts/$name" --noheadings >/dev/null; then
     current=$(findmnt --mountpoint "$base/mounts/$name" --noheadings --output SOURCE)
     test "$current" = "$loop" || fail changed_mount_source
     umount "$base/mounts/$name"
    fi
    losetup --detach "$loop"
   elif test -e "$loop" && losetup --list --noheadings "$loop" | grep -q .; then
    fail mapping_ownership_changed
   fi
   rmdir "$base/mounts/$name"
  done < <(tac "$base/loops.tsv")
 fi
 rm -f "$base/loops.tsv" "$base/ready" "$base/compatibility.tsv"
 rm -rf -- "$base/selected" "$base/dtbo" "$base/meta"
 rmdir "$base/mounts" 2>/dev/null || true
}
prepare_super() {
 local source maximum slots first flags count bytes extent name sector sectors
 source=$(partition super)
 read_region "$source" 0 12288 "$base/meta/geometry.bin"
 maximum=$(u32 little "$base/meta/geometry.bin" 4136)
 slots=$(u32 little "$base/meta/geometry.bin" 4140)
 ((maximum>=512 && maximum<=1048576 && maximum%512==0 && slots>=1 && slots<=4)) || fail geometry_bounds
 read_region "$source" 12288 "$maximum" "$base/meta/primary.bin"
 read_region "$source" "$((12288+maximum*slots))" "$maximum" "$base/meta/backup.bin"
 cmp -s "$base/meta/primary.bin" "$base/meta/backup.bin" || fail metadata_copy_mismatch
 bash "$lib/read-super.sh" "$base/meta/geometry.bin" "$base/meta/primary.bin" "$(blockdev --getsize64 "$source")" > "$base/meta/partitions.tsv"
 for name in odm_a vendor_a; do
  test "$(awk -F '\t' -v name="$name" '$1==name {n++} END {print n+0}' "$base/meta/partitions.tsv")" = 1 || fail ambiguous_logical_partition
  IFS=$'\t' read -r name flags count bytes extent < <(awk -F '\t' -v name="$name" '$1==name' "$base/meta/partitions.tsv")
  [[ $flags = 1 && $count = 1 && $extent =~ ^[0-9]+:[0-9]+$ ]] || fail snapshot_or_nonlinear_partition
  sector=${extent%:*}; sectors=${extent#*:}
  test "$bytes" = "$((sectors*512))" || fail extent_length
  mount_region "$name" "$source" "$((sector*512))" "$bytes" ext4
 done
 # Refuse concurrent metadata movement; no update/snapshot operation is invoked.
 read_region "$source" 12288 "$maximum" "$base/meta/after.bin"
 cmp -s "$base/meta/primary.bin" "$base/meta/after.bin" || fail metadata_changed
}
prepare_dtbo() {
 local label source header total entries count size offset capacity
 for label in dtbo_a dtbo_b; do
  source=$(partition "$label"); header=$base/meta/$label.header
  read_region "$source" 0 65536 "$header"
  test "$(u32 big "$header" 0)" = 3619138334 || fail dtbo_magic
  total=$(u32 big "$header" 4); entries=$(u32 big "$header" 20); count=$(u32 big "$header" 16)
  test "$(u32 big "$header" 8)" = 32 && test "$(u32 big "$header" 12)" = 32 && test "$(u32 big "$header" 28)" = 0 || fail dtbo_layout
  ((count==1 && entries>=32 && entries+32<=65536)) || fail unsupported_dtbo_table
  size=$(u32 big "$header" "$entries"); offset=$(u32 big "$header" "$((entries+4))")
  capacity=$(blockdev --getsize64 "$source")
  ((total<=capacity && size>=40 && size<=4194304 && offset>=entries+32 && offset<=total-size)) || fail dtbo_bounds
  read_region "$source" "$offset" "$size" "$base/dtbo/$label.dtb"
  test "$(u32 big "$base/dtbo/$label.dtb" 0)" = 3490578157 && test "$(u32 big "$base/dtbo/$label.dtb" 4)" = "$size" || fail fdt_header
 done
}
prepare_selected() {
 local file name candidate plugin='' destination count=0
 destination=$base/selected
 mkdir "$destination" "$destination/odm"
 # Compare the complete installed defaults before admitting a native profile.
 for file in /usr/share/uke-sensors/odm/*; do
  name=${file##*/}; [[ $name =~ ^[a-zA-Z0-9_.-]+$ ]] || return 1
  test -f "$file" && test ! -L "$file" || return 1
  candidate=$base/mounts/odm_a/etc/sensors/config/$name
  test -f "$candidate" && test ! -L "$candidate" && cmp -s "$file" "$candidate" || return 1
  cp -- "$candidate" "$destination/odm/$name"
  cmp -s "$candidate" "$destination/odm/$name" || return 1
  count=$((count+1))
 done
 ((count>0 && count<=256)) && test -f "$destination/odm/json.lst" || return 1
 for candidate in "$base"/mounts/dsp_{a,b}/adsp/sns_tppe.so; do
  if test -f "$candidate" && test ! -L "$candidate" && cmp -s "$candidate" /usr/lib/firmware/qcom/uke/sensors/sns_tppe.so; then plugin=$candidate; break; fi
 done
 test -n "$plugin" || return 1
 cp -- "$plugin" "$destination/sns_tppe.so"
 cmp -s "$plugin" "$destination/sns_tppe.so" || return 1
 printf 'selected_ssc_defaults\t%s\tbyte_identical\nselected_ssc_plugin\t1\tbyte_identical\n' "$count" >> "$base/compatibility.tsv"
}
compare_firmware() {
 local file candidate relative matches=0
 for file in /usr/lib/firmware/adsp.* /usr/lib/firmware/adsp_dtb.* /usr/lib/firmware/qca6750/wpss.*; do
  test -f "$file" && test ! -L "$file" || continue
  relative=${file#/usr/lib/firmware/}
  for candidate in "$base/mounts/modem_a/image/$relative" "$base/mounts/modem_b/image/$relative"; do
   if test -f "$candidate" && test ! -L "$candidate" && cmp -s "$file" "$candidate"; then matches=$((matches+1)); break; fi
  done
 done
 printf 'installed_modem_wlan_adsp_matches\t%s\tpackaged_consumer_retained\n' "$matches" >> "$base/compatibility.tsv"
}
test "$(id -u)" = 0 || fail root_required
exec 9>/run/lock/uke-oem-resources.lock
flock -x 9
case ${1:-} in
 start)
  tr '\0' '\n' < /proc/device-tree/compatible | grep -Fxq xiaomi,uke || fail unsupported_device
  test ! -e "$base/loops.tsv" || fail already_prepared_or_cleanup_required
  mkdir -p "$base" "$base/mounts" "$base/meta" "$base/dtbo"
  test ! -L "$base" && test "$(stat -c %u "$base")" = 0 && test "$(stat -c %a "$base")" = 700 || fail unsafe_runtime_directory
  trap 'result=$?; if ((result)); then stop || true; fi; exit "$result"' EXIT
  for label in modem_a modem_b dsp_a dsp_b bluetooth_a bluetooth_b; do
   source=$(partition "$label"); bytes=$(blockdev --getsize64 "$source")
   ((bytes>0 && bytes<=536870912)) || fail physical_size_bounds
   type=vfat; [[ $label != dsp_* ]] || type=ext4
   mount_region "$label" "$source" 0 "$bytes" "$type"
  done
  prepare_super
  prepare_dtbo
  if ! prepare_selected; then rm -rf -- "$base/selected"; printf 'selected_ssc_defaults\t0\tpackage_fallback\n' >> "$base/compatibility.tsv"; fi
  compare_firmware
  touch "$base/ready"
  cat "$base/compatibility.tsv"
  ;;
 stop) stop;;
 status) test -f "$base/ready"; cat "$base/compatibility.tsv"; findmnt --list --raw --noheadings --output TARGET,FSTYPE,OPTIONS | awk -v prefix="$base/mounts/" 'index($1,prefix)==1';;
 *) fail 'Usage: resources {start|stop|status}';;
esac
