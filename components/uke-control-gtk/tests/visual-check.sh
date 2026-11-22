#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
set -euo pipefail
# Invoke inside a nonroot Xvfb/dbus session. Screenshots are laboratory evidence.
component=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
executable=${1:-"$component/build/uke-control-gtk"}
fixture=${2:-"$component/../uke-control/tests/fixtures/tui.json"}
output="$component/build/screenshots"
mkdir -p "$output"
wait_window() {
    for ((attempt=0; attempt<50; ++attempt)); do
        window=$(xdotool search --onlyvisible --pid "$app_pid" --name 'Uke Control' 2>/dev/null | head -n1 || true)
        [[ -z $window ]] || return 0
        sleep .1
    done
    echo 'No native application window appeared' >&2
    return 1
}
close_app() {
    xdotool windowfocus "$window" key ctrl+q
    wait "$app_pid"
}
"$executable" --fixture "$fixture" &
app_pid=$!
trap 'kill "$app_pid" 2>/dev/null || true' EXIT
wait_window
sleep .4
magick import -window "$window" "$output/status-home.png"
close_app
"$executable" --fixture "$fixture" --start charge.mode &
app_pid=$!
wait_window
sleep .4
magick import -window "$window" "$output/charging-request.png"
xdotool windowsize "$window" 700 760
sleep .4
magick import -window "$window" "$output/charging-narrow.png"
close_app
"$executable" --read-only --smoke-test &
app_pid=$!
wait_window
sleep 1
magick import -window "$window" "$output/live-provider-unavailable.png"
wait "$app_pid"
trap - EXIT
printf 'Native GTK screenshots saved under %s\n' "$output"
