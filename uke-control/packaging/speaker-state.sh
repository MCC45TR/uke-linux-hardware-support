# SPDX-License-Identifier: MIT
# Embedded in RPM pre/posttrans, with phase assigned by the recipe.
# No private calibration, audio command, unit trigger or background process.
set -eu
set -f
export LC_ALL=C
state_dir=/run/uke-control-speaker-migration
template=uke-speaker-provision@.service
trap 'result=$?; if [ "$result" != 0 ]; then printf "%s\n" "Uke speaker ownership migration refused; inspect the retained transaction record before retrying." >&2; fi' EXIT
[ "$(/usr/bin/id -u)" = 0 ] || exit 1
[ -d /run/systemd/system ] || exit 0

valid_enabled() {
    case "$1" in enabled|enabled-runtime|disabled|static|indirect|masked|masked-runtime) return 0;; *) return 1;; esac
}
valid_unit() {
    printf '%s\n' "$1" | /usr/bin/grep -Eq '^uke-speaker-provision@controlC[0-9]{1,3}[.]service$'
}
enabled_state() {
    value=$(/usr/bin/systemctl is-enabled "$1" 2>/dev/null) || :
    valid_enabled "$value" || return 1
    printf '%s\n' "$value"
}
restore_enabled() {
    unit=$1; enabled=$2
    case "$enabled" in
        enabled) /usr/bin/systemctl --no-reload enable "$unit";;
        enabled-runtime) /usr/bin/systemctl --no-reload enable --runtime "$unit";;
        masked|masked-runtime)
            # Removal must not silently unmask a user's explicit preference.
            [ "$(enabled_state "$unit")" = "$enabled" ];;
        disabled|static|indirect) :;;
        *) return 1;;
    esac
}

case "$phase" in
save)
    old=$(/usr/bin/rpm -q --qf '%{NAME}' uke-speaker-runtime 2>/dev/null) || exit 0
    [ "$old" = uke-speaker-runtime ] || exit 0
    [ ! -e "$state_dir" ] && [ ! -L "$state_dir" ] || exit 1
    template_state=$(enabled_state "$template") || exit 1
    # Read and validate all states before creating a transaction record.
    rows=$(/usr/bin/systemctl list-units --all --type=service --no-legend --plain 'uke-speaker-provision@*.service') || exit 1
    snapshot=''
    count=0
    for unit in $(printf '%s\n' "$rows" | /usr/bin/awk 'NF {print $1}'); do
        valid_unit "$unit" || exit 1
        count=$((count+1)); [ "$count" -le 16 ] || exit 1
        enabled=$(enabled_state "$unit") || exit 1
        active=$(/usr/bin/systemctl show --property=ActiveState --value "$unit") || exit 1
        case "$active" in active|inactive|failed) :;; *) exit 1;; esac
        if [ "$active" = active ]; then
            case "$template_state:$enabled" in *masked*) exit 1;; esac
        fi
        snapshot="$snapshot$unit $active $enabled
"
    done
    /usr/bin/install -d -m700 "$state_dir"
    [ "$(/usr/bin/stat -c '%u:%g:%a' "$state_dir")" = 0:0:700 ] || exit 1
    temporary=$(/usr/bin/mktemp "$state_dir/.units.XXXXXX") || exit 1
    (umask 077; printf 'V1 %s %s\n%s' "$template_state" "$(/usr/bin/date +%s)" "$snapshot" > "$temporary")
    /usr/bin/chmod 600 "$temporary"
    /usr/bin/mv -T "$temporary" "$state_dir/units"
    ;;
restore)
    [ -e "$state_dir" ] || { [ ! -L "$state_dir" ] && exit 0; exit 1; }
    [ -d "$state_dir" ] && [ ! -L "$state_dir" ] || exit 1
    [ "$(/usr/bin/stat -c '%u:%g:%a' "$state_dir")" = 0:0:700 ] || exit 1
    record=$state_dir/units
    [ -f "$record" ] && [ ! -L "$record" ] || exit 1
    [ "$(/usr/bin/stat -c '%u:%g:%a:%h' "$record")" = 0:0:600:1 ] || exit 1
    [ "$(/usr/bin/stat -c %s "$record")" -le 4096 ] || exit 1
    # Validate the complete record before any systemd mutation. No eval/source.
    /usr/bin/awk '
      NR==1 {if(NF!=3 || $1!="V1" || $2!~/^(enabled|enabled-runtime|disabled|static|indirect|masked|masked-runtime)$/ || $3!~/^[1-9][0-9]{0,15}$/)bad=1;next}
      {if(NF!=3 || $1!~/^uke-speaker-provision@controlC[0-9]{1,3}[.]service$/ || seen[$1]++ || $2!~/^(active|inactive|failed)$/ || $3!~/^(enabled|enabled-runtime|disabled|static|indirect|masked|masked-runtime)$/ || ($2=="active" && $3~/masked/))bad=1}
      END{if(NR<1 || NR>17)bad=1;exit bad}' "$record" || exit 1
    IFS=' ' read -r version template_state timestamp < "$record"
    now=$(/usr/bin/date +%s)
    [ "$timestamp" -le "$now" ] && [ "$((now-timestamp))" -le 3600 ] || exit 1
    if [ "$template_state" = masked ] || [ "$template_state" = masked-runtime ]; then
        ! /usr/bin/grep -Eq '^[^ ]+ active ' "$record" || exit 1
    fi
    /usr/bin/systemctl daemon-reload
    restore_enabled "$template" "$template_state" || exit 1
    # The service's native --ensure-loaded takes the authenticated control lock
    # and returns without private-source reads/writes in healthy staged states.
    /usr/bin/tail -n +2 "$record" | while IFS=' ' read -r unit active enabled; do
        restore_enabled "$unit" "$enabled" || exit 1
        case "$enabled" in masked|masked-runtime) continue;; esac
        [ "$active" != active ] || /usr/bin/systemctl start "$unit" || exit 1
    done
    /usr/bin/rm -- "$record"
    /usr/bin/rmdir -- "$state_dir"
    ;;
*) exit 1;;
esac
