# SPDX-License-Identifier: MIT
# Embedded in Control RPM pre/posttrans. No hardware or TEE operation.
set -eu
export LC_ALL=C
state_dir=/run/uke-control-pogo-migration
[ "$(/usr/bin/id -u)" = 0 ] || exit 1
[ -d /run/systemd/system ] || exit 0
valid_enabled() {
 case "$1" in enabled|enabled-runtime|disabled|static|indirect|masked|masked-runtime) return 0;; *) return 1;; esac
}
valid_active() {
 case "$1" in active|inactive|failed|activating|deactivating|reloading) return 0;; *) return 1;; esac
}
control() {
 scope=$1;shift
 case "$scope" in
  system) /usr/bin/systemctl "$@" uke-pogo-auth.service;;
  user-*) session_uid=${scope#user-};case "$session_uid" in ''|*[!0-9]*) return 1;;esac
   /usr/bin/systemctl --user --machine="$session_uid@.host" "$@" uke-pogo-desktop.service;;
  *) return 1;;
 esac
}
private_directory() {
 [ -d "$state_dir" ] && [ ! -L "$state_dir" ] &&
 [ "$(/usr/bin/stat -c %u "$state_dir")" = 0 ] &&
 [ "$(/usr/bin/stat -c %a "$state_dir")" = 700 ]
}
snapshot() {
 target=$1
 enabled=$(control "$target" is-enabled 2>/dev/null) || :
 active=$(control "$target" is-active 2>/dev/null) || :
 valid_enabled "$enabled" && valid_active "$active" || return 1
 (umask 077;printf '%s\n%s\n' "$enabled" "$active" > "$state_dir/$target")
}
case "$phase" in
 save)
  auth=$(/usr/bin/rpm -qf --qf '%{NAME}' /usr/libexec/uke-pogo-auth 2>/dev/null) || :
  desktop=$(/usr/bin/rpm -qf --qf '%{NAME}' /usr/libexec/uke-pogo-desktop 2>/dev/null) || :
  case "$auth" in uke-pogo-auth|uke-control) auth=yes;; *) auth=no;;esac
  case "$desktop" in uke-pogo-desktop|uke-control) desktop=yes;; *) desktop=no;;esac
  [ "$auth" = yes ] || [ "$desktop" = yes ] || exit 0
  [ ! -e "$state_dir" ] && [ ! -L "$state_dir" ] || exit 1
  /usr/bin/mkdir -m700 "$state_dir"
  [ "$auth" != yes ] || snapshot system
  if [ "$desktop" = yes ];then
   for session_dir in /run/user/*;do
    [ -d "$session_dir" ] && [ ! -L "$session_dir" ] && [ -S "$session_dir/bus" ] || continue
    session_uid=${session_dir##*/};case "$session_uid" in ''|*[!0-9]*) continue;;esac
    snapshot "user-$session_uid"
   done
  fi
  (umask 077;printf '1\n' > "$state_dir/complete")
  ;;
 restore)
  [ -e "$state_dir" ] || [ -L "$state_dir" ] || exit 0
  private_directory || exit 1
  [ -f "$state_dir/complete" ] && [ ! -L "$state_dir/complete" ] &&
   [ "$(/usr/bin/cat "$state_dir/complete")" = 1 ] || exit 1
  /usr/bin/systemctl daemon-reload || exit 1
  for entry in "$state_dir"/*;do
   [ "$entry" != "$state_dir/complete" ] || continue
   [ -f "$entry" ] && [ ! -L "$entry" ] && [ "$(/usr/bin/stat -c %u "$entry")" = 0 ] &&
    [ "$(/usr/bin/stat -c %a "$entry")" = 600 ] || exit 1
   target=${entry##*/}
   case "$target" in
    system) ;;
    user-*) session_uid=${target#user-};case "$session_uid" in ''|*[!0-9]*) exit 1;;esac
     # A session which logged out cannot be restarted by a package transaction.
     [ -S "/run/user/$session_uid/bus" ] || { /usr/bin/unlink "$entry";continue; }
     /usr/bin/systemctl --user --machine="$session_uid@.host" daemon-reload || exit 1;;
    *) exit 1;;
   esac
   [ "$(/usr/bin/wc -l < "$entry")" = 2 ] || exit 1
   enabled=$(/usr/bin/sed -n '1p' "$entry")
   active=$(/usr/bin/sed -n '2p' "$entry")
   valid_enabled "$enabled" && valid_active "$active" || exit 1
   current=$(control "$target" is-enabled 2>/dev/null) || :
   valid_enabled "$current" || exit 1
   case "$enabled:$current" in *masked*) /usr/bin/unlink "$entry";continue;;esac
   case "$enabled" in
    enabled) control "$target" enable || exit 1;;
    enabled-runtime) control "$target" enable --runtime || exit 1;;
   esac
   case "$active" in active|activating|reloading)
    running=$(control "$target" is-active 2>/dev/null) || :
    case "$running" in
     active|activating|reloading) control "$target" try-restart || exit 1;;
     inactive|failed) control "$target" start || exit 1;;
     deactivating) :;;
     *) exit 1;;
    esac
   ;;esac
   /usr/bin/unlink "$entry"
  done
  /usr/bin/unlink "$state_dir/complete"
  /usr/bin/rmdir "$state_dir"
  ;;
 *) exit 1;;
esac
