#!/usr/bin/bash
set -Eeuo pipefail
[[ $# == 1 ]]
source_file=$(realpath -e -- "$1")
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
mkdir "$work/manager" "$work/dev"
ln -s /dev/null "$work/dev/ttyGS3"
sed -n '/^owned() {/,/^recovery_stop() {/p' "$source_file" | sed '$d' |
    sed -e 's@/run/systemd/system@$work/manager@g' -e 's@/dev/\$line@$work/dev/$line@g' > "$work/functions"
source "$work/functions"
events=$work/events
stub_systemctl() {
    case "$1" in
        show) [[ $* == 'show --value --property=ActiveState serial-getty@ttyGS3.service' ]]
              printf '%s\n' "$getty_state";;
        stop) [[ $* == 'stop serial-getty@ttyGS3.service' ]]
              [[ -z $(cat "$g/UDC") && -L $g/configs/c.1/acm.gs0 ]]
              printf 'stop ttyGS3\n' >> "$events"
              return "$stop_result";;
        is-enabled) [[ $* == 'is-enabled serial-getty@ttyGS3.service' ]]
                    printf '%s\n' "$enabled"
                    [[ $enabled == enabled || $enabled == enabled-runtime ]];;
        --no-block) [[ $* == '--no-block start serial-getty@ttyGS3.service' ]]
                    printf 'start ttyGS3\n' >> "$events";;
        *) return 92;;
    esac
}
systemctl() { stub_systemctl "$@"; }
timeout() {
    [[ $1 == --kill-after=1s ]]
    shift 2
    case $1 in
        systemctl) shift; stub_systemctl "$@";;
        fuser)
            [[ $* == "fuser -s $work/dev/ttyGS3" ]]
            printf 'check ttyGS3\n' >> "$events"
            if [[ -n $busy_diagnostic ]]; then printf '%s\n' "$busy_diagnostic" >&2; fi
            return "$busy";;
        *) return 93;;
    esac
}
rmdir() {
    dir=${!#}
    if [[ $dir == "$g/functions/acm.gs0" ]]; then
        printf 'remove acm\n' >> "$events"
        grep -Fxq 'check ttyGS3' "$events" || return 94
        [[ $getty_state == inactive ]] || grep -Fxq 'stop ttyGS3' "$events" || return 95
        unlink "$dir/port_num"
    elif [[ $dir == "$g" ]]; then
        unlink "$g/UDC"
        command rmdir "$g/functions" "$g/configs"
    fi
    command rmdir "$@"
}
setup() {
    state=$work/state-$1
    g=$work/gadget-$1
    mkdir -p "$state" "$g"/{configs/c.1,functions/acm.gs0}
    stat -c '%d:%i' "$g" > "$state/usb-gadget-owner"
    printf 'manual-acm\n' > "$state/usb-gadget-profile"
    printf 'fixture-udc\n' > "$g/UDC"
    printf '3\n' > "$g/functions/acm.gs0/port_num"
    ln -s "$g/functions/acm.gs0" "$g/configs/c.1/acm.gs0"
    : > "$events"
    getty_state=active
    enabled=enabled
    stop_result=0
    busy=1
    busy_diagnostic=
}
refused() {
    if cleanup; then exit 90; fi
    [[ -d $g/functions/acm.gs0 && -L $g/configs/c.1/acm.gs0 && -s $state/usb-gadget-owner ]]
}
setup getty
cleanup
[[ ! -e $g && ! -e $state/usb-gadget-owner ]]
cmp "$events" <(printf 'stop ttyGS3\ncheck ttyGS3\nremove acm\n')
setup open-local
busy=0
refused
cmp "$events" <(printf 'stop ttyGS3\ncheck ttyGS3\n')
setup getty-timeout
stop_result=124
refused
cmp "$events" <(printf 'stop ttyGS3\n')
setup fuser-timeout
getty_state=inactive
busy=124
refused
cmp "$events" <(printf 'check ttyGS3\n')
setup fuser-syntax-failure
busy=1
busy_diagnostic='No process specification given'
refused
cmp "$events" <(printf 'stop ttyGS3\ncheck ttyGS3\n')
setup fuser-permission-failure
busy=1
busy_diagnostic='Permission denied'
refused
cmp "$events" <(printf 'stop ttyGS3\ncheck ttyGS3\n')
setup malformed-port
printf '%s\n' '../../../not-a-port' > "$g/functions/acm.gs0/port_num"
refused
[[ ! -s $events ]]
setup foreign
mkdir "$g/functions/foreign.0"
refused
[[ ! -s $events && $(cat "$g/UDC") == fixture-udc ]]
setup configured
start_configured_acm_getty
cmp "$events" <(printf 'start ttyGS3\n')
enabled=disabled
: > "$events"
start_configured_acm_getty
[[ ! -s $events ]]
setup ecm-only
unlink "$g/configs/c.1/acm.gs0"
unlink "$g/functions/acm.gs0/port_num"
command rmdir "$g/functions/acm.gs0"
printf 'recovery-ecm\n' > "$state/usb-gadget-profile"
mkdir "$g/functions/ecm.usb0"
ln -s "$g/functions/ecm.usb0" "$g/configs/c.1/ecm.usb0"
cleanup
[[ ! -e $g && ! -s $events ]]
printf 'ACM teardown order, wait bounds, busy TTY, ownership and enabled-getty restart passed.\n'
