#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
set -Eeuo pipefail
mapfile -t daemon < <(dbus-daemon --session --fork --print-address=1 --print-pid=1)
test "${#daemon[@]}" = 2
[[ ${daemon[1]} =~ ^[0-9]+$ ]]
trap 'kill "${daemon[1]}" 2>/dev/null || true' EXIT
export DBUS_SYSTEM_BUS_ADDRESS=${daemon[0]}
timeout --signal=KILL 20 build/test-sensor-proxy
