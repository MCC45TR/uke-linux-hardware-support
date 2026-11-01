#!/bin/sh
# Run only in an admitted SSC laboratory session with the distribution proxy stopped.
set -eu
test "$#" -eq 3 || { echo 'Usage: sh iio-startup-hil.sh PROXY MONITOR LIBDIR' >&2; exit 2; }
proxy=$1
monitor=$2
library=$3
test -x "$proxy" && test -x "$monitor" && test -d "$library"
if pidof "$proxy" >/dev/null || busctl --system --no-pager --no-legend list | grep -q '^net.hadess.SensorProxy '; then
    echo 'A SensorProxy already owns the name; refusing overlapping trials.' >&2
    exit 1
fi
trial=$(mktemp -d)
proxy_pid=
monitor_pid=
cleanup() {
    test -z "$monitor_pid" || kill "$monitor_pid" 2>/dev/null || :
    test -z "$proxy_pid" || kill "$proxy_pid" 2>/dev/null || :
    wait 2>/dev/null || :
    echo "Private trial logs: $trial"
}
trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' TERM
timeout -s TERM -k 2 15 "$monitor" --accel > "$trial/monitor.log" 2>&1 &
monitor_pid=$!
# Start the client first to exercise the startup ordering, rather than delaying it.
sleep 1
grep -q 'Waiting for iio-sensor-proxy to appear' "$trial/monitor.log"
LD_LIBRARY_PATH="$library" timeout -s TERM -k 2 20 "$proxy" --verbose > "$trial/proxy.log" 2>&1 &
proxy_pid=$!
rc=0
wait "$monitor_pid" || rc=$?
monitor_pid=
test "$rc" -eq 124
cat "$trial/monitor.log"
grep -Eq '(orientation: |orientation changed: )(normal|bottom-up|left-up|right-up)' "$trial/monitor.log"
if grep -Eq 'Failed to claim|CRITICAL|ERROR' "$trial/monitor.log"; then
    exit 1
fi
kill "$proxy_pid"
wait "$proxy_pid" || :
proxy_pid=
if grep -Eq 'CRITICAL|assertion .*failed|segmentation fault' "$trial/proxy.log"; then
    exit 1
fi
echo 'PASS: a client waiting before startup received a valid orientation.'
