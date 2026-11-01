#!/bin/bash
# SPDX-License-Identifier: MIT
set -Eeuo pipefail
export LC_ALL=C PATH=/usr/sbin:/usr/bin

trace_session() (
    root=$1
    instance=$root/instances/uke_shutdown
    created=0 reader= owned=
    [[ ! -e $instance ]] || { printf 'Existing Uke trace instance preserved\n' >&2; exit 1; }
    found=0
    grep -Eq '^[pr][0-9]*:uke_shutdown/' "$root/kprobe_events" || found=$?
    ((found == 1)) || { printf 'Existing or unreadable Uke probe definitions preserved\n' >&2; exit 1; }
    for symbol in usb_role_switch_set_role dwc3_set_mode dwc3_host_exit; do
        grep -Eq "^$symbol( |$)" "$root/available_filter_functions" || {
            printf 'Required trace symbol unavailable: %s\n' "$symbol" >&2; exit 1;
        }
    done
    cleanup() {
        status=$?
        trap - EXIT INT TERM HUP
        if [[ -n $reader ]]; then
            kill -TERM "$reader" 2>/dev/null || :
            wait "$reader" 2>/dev/null || :
        fi
        if ((created)); then
            printf '0\n' > "$instance/tracing_on" || status=1
            if [[ -f $instance/events/uke_shutdown/enable ]]; then
                printf '0\n' > "$instance/events/uke_shutdown/enable" || status=1
            fi
            rmdir "$instance" || status=1
        fi
        for event in $owned; do
            printf -- '-:uke_shutdown/%s\n' "$event" >> "$root/kprobe_events" || status=1
        done
        exit "$status"
    }
    trap cleanup EXIT
    trap 'exit 0' INT TERM HUP
    mkdir "$instance"
    created=1
    printf '0\n' > "$instance/tracing_on"
    printf '64\n' > "$instance/buffer_size_kb"
    printf 'mono\n' > "$instance/trace_clock"
    printf 'p:uke_shutdown/role usb_role_switch_set_role role=$arg2:u32\n' >> "$root/kprobe_events"
    owned=role
    printf 'p:uke_shutdown/mode dwc3:dwc3_set_mode mode=$arg2:u32\n' >> "$root/kprobe_events"
    owned+=' mode'
    printf 'p:uke_shutdown/host_exit dwc3:dwc3_host_exit\n' >> "$root/kprobe_events"
    owned+=' host_exit'
    for event in $owned; do
        printf 'stacktrace:32\n' > "$instance/events/uke_shutdown/$event/trigger"
        printf '1\n' > "$instance/events/uke_shutdown/$event/enable"
    done
    printf '1\n' > "$instance/tracing_on"
    printf 'Uke shutdown trace ready: isolated buffer, three probes, at most 96 stack traces; no USB control writes\n'
    cat "$instance/trace_pipe" &
    reader=$!
    wait "$reader"
)

if [[ ${BASH_SOURCE[0]} == "$0" ]]; then
    [[ $# == 0 && $(stat -f -c %T /sys/kernel/tracing) == tracefs ]]
    tr '\0' '\n' < /sys/firmware/devicetree/base/compatible | grep -Fxq xiaomi,uke
    tr '\0' '\n' < /sys/firmware/devicetree/base/compatible | grep -Fxq qcom,sm7675
    trace_session /sys/kernel/tracing
fi
