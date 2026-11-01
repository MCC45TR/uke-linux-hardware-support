#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
set -euo pipefail
# Run as a normal user. No mutations or terminal handoffs are performed.
component=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
executable=${1:-"$component/build/uke-control-gtk"}
fixture=${2:-"$component/../uke-control/tests/fixtures/tui.json"}
if [[ $(id -u) == 0 ]]; then
    echo 'Run the graphical smoke test as a normal user.' >&2
    exit 77
fi
export GDK_BACKEND=x11 GSK_RENDERER=cairo GTK_A11Y=none
export NO_AT_BRIDGE=1 GTK_USE_PORTAL=0
exec xvfb-run -n "${UKE_GTK_TEST_DISPLAY:-91}" -s '-screen 0 1440x1000x24' \
    dbus-run-session -- "$executable" --fixture "$fixture" --smoke-test
