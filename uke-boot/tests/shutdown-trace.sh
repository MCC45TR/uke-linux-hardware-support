#!/bin/bash
set -Eeuo pipefail
cd -- "$(dirname -- "$0")/.."
source src/shutdown-trace.sh
fixture=$(mktemp -d)
trap 'find "$fixture" -depth -delete' EXIT
mkdir -p "$fixture/instances"
printf 'foreign probe preserved\n' > "$fixture/kprobe_events"
printf 'usb_role_switch_set_role\ndwc3_set_mode [dwc3]\ndwc3_host_exit [dwc3]\n' > "$fixture/available_filter_functions"
broken=0 linger=0
mkdir() {
    command mkdir "$@"
    [[ $1 == "$fixture/instances/uke_shutdown" ]]
    for event in role mode host_exit; do
        command mkdir -p "$1/events/uke_shutdown/$event"
        : > "$1/events/uke_shutdown/$event/trigger"
        : > "$1/events/uke_shutdown/$event/enable"
    done
    : > "$1/events/uke_shutdown/enable"
    printf '%s\n' "$BASHPID" > "$fixture/worker-pid"
    if ((linger)); then
        mkfifo "$1/trace_pipe"
    else
        printf 'synthetic trace event\n' > "$1/trace_pipe"
    fi
    if ((broken)); then
        unlink "$1/events/uke_shutdown/mode/trigger"
        command mkdir "$1/events/uke_shutdown/mode/trigger"
    fi
}
rmdir() {
    [[ $1 == "$fixture/instances/uke_shutdown" ]]
    cp "$1/buffer_size_kb" "$fixture/recorded-buffer"
    cp "$1/trace_clock" "$fixture/recorded-clock"
    for event in role mode host_exit; do
        if [[ -f $1/events/uke_shutdown/$event/trigger ]]; then
            cp "$1/events/uke_shutdown/$event/trigger" "$fixture/recorded-$event"
        fi
    done
    find "$1" -depth -delete
}
export fixture broken linger
export -f trace_session mkdir rmdir
run_session() { bash -c 'set -Eeuo pipefail; trace_session "$fixture"'; }
bash -n src/shutdown-trace.sh
run_session > "$fixture/trace-output"
grep -Fxq 'synthetic trace event' "$fixture/trace-output"
test "$(cat "$fixture/recorded-buffer")" = 64
test "$(cat "$fixture/recorded-clock")" = mono
for event in role mode host_exit; do
    test "$(cat "$fixture/recorded-$event")" = stacktrace:32
    grep -Fxq -- "-:uke_shutdown/$event" "$fixture/kprobe_events"
done
test ! -e "$fixture/instances/uke_shutdown"
grep -Fxq 'foreign probe preserved' "$fixture/kprobe_events"
before=$(sha256sum "$fixture/kprobe_events")
command mkdir "$fixture/instances/uke_shutdown"
if run_session > /dev/null 2>&1; then exit 1; fi
test "$(sha256sum "$fixture/kprobe_events")" = "$before"
command rmdir "$fixture/instances/uke_shutdown"
mv "$fixture/kprobe_events" "$fixture/saved-probes"
command mkdir "$fixture/kprobe_events"
if run_session > /dev/null 2>&1; then exit 1; fi
test ! -e "$fixture/instances/uke_shutdown"
command rmdir "$fixture/kprobe_events"
mv "$fixture/saved-probes" "$fixture/kprobe_events"
printf 'p:uke_shutdown/foreign original_symbol\n' >> "$fixture/kprobe_events"
before=$(sha256sum "$fixture/kprobe_events")
if run_session > /dev/null 2>&1; then exit 1; fi
test "$(sha256sum "$fixture/kprobe_events")" = "$before"
printf 'foreign probe preserved\n' > "$fixture/kprobe_events"
printf 'usb_role_switch_set_role\n' > "$fixture/available_filter_functions"
if run_session > /dev/null 2>&1; then exit 1; fi
test ! -e "$fixture/instances/uke_shutdown"
printf 'usb_role_switch_set_role\ndwc3_set_mode [dwc3]\ndwc3_host_exit [dwc3]\n' > "$fixture/available_filter_functions"
broken=1
if run_session > "$fixture/failed-setup" 2>&1; then exit 1; fi
test ! -e "$fixture/instances/uke_shutdown"
for event in role mode host_exit; do
    grep -Fxq -- "-:uke_shutdown/$event" "$fixture/kprobe_events"
done
grep -Fxq 'foreign probe preserved' "$fixture/kprobe_events"
printf 'foreign probe preserved\n' > "$fixture/kprobe_events"
broken=0 linger=1
run_session > "$fixture/signal-output" &
job=$!
for ((attempt=0;attempt<100;attempt++)); do
    if grep -Fq 'trace ready:' "$fixture/signal-output"; then break; fi
    sleep .02
done
grep -Fq 'trace ready:' "$fixture/signal-output"
worker=$(cat "$fixture/worker-pid")
[[ $worker =~ ^[0-9]+$ && $worker -gt 1 ]]
kill -TERM "$worker"
wait "$job"
test ! -e "$fixture/instances/uke_shutdown"
for event in role mode host_exit; do
    test "$(grep -Fxc -- "-:uke_shutdown/$event" "$fixture/kprobe_events")" = 1
done
printf 'Isolated trace setup, bounded stacks, foreign-state preservation, partial-setup and SIGTERM cleanup passed; regular files/FIFO only\n'
