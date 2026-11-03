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
SYSTEMD_UNIT_PATH="$stage:/usr/lib/systemd/system" systemd-analyze verify --man=no "$stage/uke-rtc-sync.service" "$stage/uke-rtc-sync.timer"
printf 'RTC unit capability/device/timing policy and syntax passed; no daemon or RTC operation\n'
