#!/usr/bin/env bash
# Source and unit syntax checks; no host service is started.
set -Eeuo pipefail
cd -- "$(dirname -- "$0")/.."
grep -Fxq 'Wants=uke-rtc-sync.timer' src/units/75-uke-clock-floor.conf
grep -Fxq 'ExecStart=/usr/libexec/senemos-uke/uke-install-profile rtc-sync' src/units/uke-rtc-sync.service
grep -Fxq 'CapabilityBoundingSet=CAP_SYS_TIME' src/units/uke-rtc-sync.service
grep -Fxq 'DeviceAllow=char-rtc rw' src/units/uke-rtc-sync.service
grep -Fxq 'TimeoutStartSec=8' src/units/uke-rtc-sync.service
grep -Fxq 'OnUnitActiveSec=30' src/units/uke-rtc-sync.timer
grep -Fq 'Provides: uke-installation-profile(rtc-secure-offset-v1) = 1' packaging/uke-boot-integration.spec
stage=$(mktemp -d)
trap 'find "$stage" -depth -delete' EXIT
cp src/units/uke-rtc-sync.service src/units/uke-rtc-sync.timer "$stage/"
sed -i 's|^ExecStart=.*|ExecStart=/usr/bin/true|' "$stage/uke-rtc-sync.service"
# The source worker is unprivileged and has no system-manager runtime directory.
# Parse the same unit directives in a private user-manager test environment.
mkdir -m700 "$stage/run"
for target in basic.target sysinit.target shutdown.target timers.target; do
 printf '[Unit]\nDefaultDependencies=no\n' > "$stage/$target"
done
for slice in app.slice user.slice -.slice; do
 printf '[Unit]\nDefaultDependencies=no\n[Slice]\n' > "$stage/$slice"
done
XDG_RUNTIME_DIR="$stage/run" SYSTEMD_UNIT_PATH="$stage" \
 systemd-analyze --user --generators=no verify --man=no "$stage/uke-rtc-sync.service" "$stage/uke-rtc-sync.timer" > "$stage/verify.log" 2>&1 || {
 cat "$stage/verify.log" >&2;exit 1;
}
if grep -Eiq 'ordering cycle|Job .*deleted' "$stage/verify.log";then cat "$stage/verify.log" >&2;exit 1;fi
printf 'RTC unit capability/device/timing policy and syntax passed; no daemon or RTC operation\n'
