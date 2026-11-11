# SPDX-License-Identifier: MIT
# Embedded in RPM pre/posttrans; no hardware access or continuous worker.
set -eu
set -f
export LC_ALL=C
state_dir=/run/uke-control-adaptive-migration
units='uke-adaptive-display.target'
[ "$(/usr/bin/id -u)" = 0 ] || exit 1
[ -d /run/systemd/system ] || exit 0
valid_enabled() {
    case "$1" in enabled|enabled-runtime|disabled|static|indirect|masked|masked-runtime) return 0;; *) return 1;; esac
}
valid_unit() {
    case "$1" in uke-adaptive-display.target) return 0;; *) return 1;; esac
}
enabled_state() {
    value=$(/usr/bin/systemctl is-enabled "$1" 2>/dev/null) || :
    valid_enabled "$value" || return 1
    printf '%s\n' "$value"
}
case "$phase" in
save)
    old=$(/usr/bin/rpm -q --qf '%{NAME}' uke-adaptive-display 2>/dev/null) || exit 0
    [ "$old" = uke-adaptive-display ] || exit 0
    [ ! -e "$state_dir" ] && [ ! -L "$state_dir" ] || exit 1
    snapshot=''
    for unit in $units;do
        enabled=$(enabled_state "$unit") || exit 1
        active=$(/usr/bin/systemctl show --property=ActiveState --value "$unit") || exit 1
        case "$active" in active|inactive|failed) :;; *) exit 1;; esac
        if [ "$active" = active ];then
            case "$enabled" in masked|masked-runtime) exit 1;; esac
        fi
        snapshot="$snapshot$unit $active $enabled
"
    done
    /usr/bin/install -d -m700 "$state_dir"
    [ "$(/usr/bin/stat -c '%u:%g:%a' "$state_dir")" = 0:0:700 ] || exit 1
    temporary=$state_dir/units.tmp
    (umask 077;printf 'v1 %s\n%s' "$(/usr/bin/date +%s)" "$snapshot" > "$temporary")
    /usr/bin/mv -T "$temporary" "$state_dir/units"
    ;;
restore)
    [ -e "$state_dir" ] || { [ ! -L "$state_dir" ] && exit 0;exit 1; }
    record=$state_dir/units
    [ -d "$state_dir" ] && [ ! -L "$state_dir" ] && [ -f "$record" ] && [ ! -L "$record" ] || exit 1
    [ "$(/usr/bin/stat -c '%u:%g:%a' "$state_dir")" = 0:0:700 ] || exit 1
    [ "$(/usr/bin/stat -c '%u:%g:%a:%h' "$record")" = 0:0:600:1 ] || exit 1
    [ "$(/usr/bin/wc -c < "$record")" -le 1024 ] || exit 1
    /usr/bin/awk '
      NR==1 {if(NF!=2 || $1!="v1" || $2!~/^[0-9]+$/)exit 1;next}
      {if(NF!=3 || seen[$1]++ || $1!~/^uke-adaptive-display[.]target$/ ||
          $2!~/^(active|inactive|failed)$/ || $3!~/^(enabled|enabled-runtime|disabled|static|indirect|masked|masked-runtime)$/ ||
          ($2=="active" && $3~/^masked/))exit 1;n++}
      END{if(NR!=2 || n!=1)exit 1}' "$record" || exit 1
    timestamp=$(/usr/bin/awk 'NR==1{print $2}' "$record")
    now=$(/usr/bin/date +%s)
    [ "$timestamp" -le "$now" ] && [ "$((now-timestamp))" -le 3600 ] || exit 1
    /usr/bin/systemctl daemon-reload
    /usr/bin/tail -n +2 "$record" | while IFS=' ' read -r unit active enabled;do
        valid_unit "$unit" || exit 1
        current=$(enabled_state "$unit") || exit 1
        # A new explicit mask during the transaction always takes precedence.
        case "$current" in masked|masked-runtime) continue;; esac
        case "$enabled" in
            enabled) /usr/bin/systemctl --no-reload enable "$unit";;
            enabled-runtime) /usr/bin/systemctl --no-reload enable --runtime "$unit";;
            masked) /usr/bin/systemctl --no-reload mask "$unit";;
            masked-runtime) /usr/bin/systemctl --no-reload mask --runtime "$unit";;
            disabled|static|indirect) :;;
        esac
        case "$enabled" in masked|masked-runtime) continue;; esac
        [ "$active" != active ] || /usr/bin/systemctl start "$unit" || exit 1
    done
    /usr/bin/rm -- "$record"
    /usr/bin/rmdir -- "$state_dir"
    ;;
*) exit 1;;
esac
