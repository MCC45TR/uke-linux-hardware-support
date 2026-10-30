#!/usr/bin/env bash
set -Eeuo pipefail
cd "$(dirname -- "$0")/.."
spec=packaging/uke-desktop-metas.spec
selection=$(awk '/^%package -n uke-cutefish-meta$/ {on=1;next} /^%description -n uke-cutefish-meta$/ {on=0} on' "$spec")
grep -Fxq 'Requires: cutefish-desktop >= 1.0^20261027' <<< "$selection"
! grep -Eq '^(Requires|Recommends):.*(kde|plasma|core-meta)' <<< "$selection"
jq -e '.provider=="cutefish-desktop" and .session=="/usr/share/wayland-sessions/cutefish-wayland.desktop" and .command=="/usr/bin/cutefish-wayland-session" and .graphical_session_tested==false and .hardware_tested==false and .automatic_session_or_display_configuration==false' src/cutefish.json >/dev/null
printf '%s\n' 'PASS: real upstream Cutefish provider, independent metadata and pending runtime gates'
