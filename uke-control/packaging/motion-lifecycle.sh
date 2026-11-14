# SPDX-License-Identifier: MIT
# Embedded package hooks. Only the owned, demand-activated Motion unit changes.
set -eu
export LC_ALL=C
state=/run/uke-control-motion-upgrade
unit=uke-control-motion.service
unit_file=/usr/lib/systemd/system/uke-control-motion.service
[ "$(/usr/bin/id -u)" = 0 ] || exit 1
[ -d /run/systemd/system ] || exit 0
valid_enabled() {
 case "$1" in enabled|enabled-runtime|disabled|static|indirect|masked|masked-runtime) return 0;;*) return 1;;esac
}
owned_unit() {
 [ -f "$unit_file" ] && [ ! -L "$unit_file" ] || return 1
 owners=$(/usr/bin/rpm -qf --qf '%{NAME}\n' "$unit_file" 2>/dev/null) || return 1
 [ -n "$owners" ] || return 1
 while IFS= read -r owner;do [ "$owner" = uke-control ] || return 1;done <<OWNERS
$owners
OWNERS
}
read_record() {
 [ -d "$state" ] && [ ! -L "$state" ] &&
 [ "$(/usr/bin/stat -c '%u:%g:%a' "$state")" = 0:0:700 ] || return 1
 [ -f "$state/unit" ] && [ ! -L "$state/unit" ] &&
 [ "$(/usr/bin/stat -c '%u:%g:%a:%h' "$state/unit")" = 0:0:600:1 ] || return 1
 [ "$(/usr/bin/find "$state" -mindepth 1 -maxdepth 1 | /usr/bin/wc -l)" = 1 ] || return 1
 [ "$(/usr/bin/wc -c < "$state/unit")" -le 256 ] &&
 [ "$(/usr/bin/wc -l < "$state/unit")" = 3 ] || return 1
 IFS=' ' read -r version saved extra < "$state/unit"
 [ "$version" = 1 ] && [ -z "$extra" ] || return 1
 case "$saved" in ''|*[!0-9]*) return 1;;esac
 [ "${#saved}" -le 10 ] || return 1
 now=$(/usr/bin/date +%s) || return 1
 [ "$saved" -le "$now" ] && [ "$((now-saved))" -le 3600 ] || return 1
 enabled=$(/usr/bin/sed -n '2p' "$state/unit")
 active=$(/usr/bin/sed -n '3p' "$state/unit")
 valid_enabled "$enabled" || return 1
 case "$active" in active|inactive|failed) :;;*) return 1;;esac
}
clear_record() { /usr/bin/unlink "$state/unit" || return 1; /usr/bin/rmdir "$state" || return 1; }
refresh_manager() {
 /usr/bin/systemctl daemon-reload || return 1
 /usr/bin/busctl --system --timeout=5 call org.freedesktop.DBus /org/freedesktop/DBus org.freedesktop.DBus ReloadConfig || return 1
}
case "$phase" in
 save-upgrade)
  case "${1-}" in ''|*[!0-9]*) exit 1;;esac
  if [ -e "$state" ] || [ -L "$state" ];then read_record || exit 1;exit 0;fi
  phase=save;;
 restore-upgrade)
  case "${1-}" in ''|*[!0-9]*) exit 1;;esac
  # A newer provider restores after its complete transaction, not here.
  if [ -e "$unit_file" ] || [ -L "$unit_file" ];then owned_unit || exit 1;exit 0;fi
  phase=restore;;
esac
case "$phase" in
 save)
  if [ -e "$state" ] || [ -L "$state" ];then exit 1;fi
  if [ ! -e "$unit_file" ] && [ ! -L "$unit_file" ];then exit 0;fi
  owned_unit || exit 1
  enabled=$(/usr/bin/systemctl is-enabled "$unit" 2>/dev/null) || :
  active=$(/usr/bin/systemctl is-active "$unit" 2>/dev/null) || :
  valid_enabled "$enabled" || exit 1
  case "$active" in active|inactive|failed) :;;*) exit 1;;esac
  /usr/bin/mkdir -m700 "$state" || exit 1
  captured=$(/usr/bin/date +%s) || exit 1
  (umask 077;printf '1 %s\n%s\n%s\n' "$captured" "$enabled" "$active" > "$state/unit") || exit 1
  read_record || exit 1
  # Stop while the outgoing executable and its recovery action still exist.
  if [ "$active" = active ];then /usr/bin/systemctl stop "$unit" || exit 1;fi
  ;;
 restore)
  has_record=no
  if [ -e "$state" ] || [ -L "$state" ];then read_record || exit 1;has_record=yes;fi
  if [ -e "$unit_file" ] || [ -L "$unit_file" ];then owned_unit || exit 1;fi
  # New installs also need the bus activation and policy configuration re-read.
  refresh_manager || exit 1
  if [ "$has_record" = no ];then exit 0;fi
  if [ ! -e "$unit_file" ] && [ ! -L "$unit_file" ];then clear_record || exit 1;exit 0;fi
  current=$(/usr/bin/systemctl is-enabled "$unit" 2>/dev/null) || :
  valid_enabled "$current" || exit 1
  case "$enabled:$current" in *masked*) clear_record || exit 1;exit 0;;esac
  if [ "$active" = active ];then
   running=$(/usr/bin/systemctl is-active "$unit" 2>/dev/null) || :
   case "$running" in
    active) :;;
    inactive|failed) /usr/bin/systemctl start "$unit" || exit 1;;
    *) exit 1;;
   esac
  fi
  clear_record || exit 1
  ;;
 *) exit 1;;
esac
