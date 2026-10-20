#!/usr/bin/env bash
# Host-only state fixtures; never invokes the real system service manager.
set -Eeuo pipefail
root=$(cd -- "$(dirname -- "$0")/../../.."; pwd)
stage=$(mktemp -d)
trap 'rm -r -- "$stage"' EXIT
mkdir -p "$stage/bin" "$stage/systemd"
sed -e "s#/run/systemd/system#$stage/systemd#g" -e "s#/run/uke-rpm-migration#$stage/state#g" -e 's#/usr/bin/selinuxenabled#mockselinuxenabled#g;s#/usr/bin/semodule#mocksemodule#g' "$root/packaging/families/host/service-rename.sh" >"$stage/fragment"
cat >"$stage/bin/rpm" <<'SH'
#!/bin/sh
printf '%s' "${FIXTURE_OLD-upower}"
SH
cat >"$stage/bin/systemctl" <<'SH'
#!/bin/sh
printf '%s\n' "$*" >>"$FIXTURE_LOG"
case $1 in
is-enabled) printf '%s\n' "$FIXTURE_ENABLED";;
is-active) printf '%s\n' "$FIXTURE_ACTIVE";;
start) test "${FIXTURE_START_FAIL-0}" = 0;;
esac
SH
chmod 755 "$stage/bin/"*
printf '#!/bin/sh\nexit "${FIXTURE_SELINUX-1}"\n' >"$stage/bin/mockselinuxenabled"
printf '#!/bin/sh\nprintf "policy:%%s\\n" "$*" >>"$FIXTURE_LOG"\n' >"$stage/bin/mocksemodule"
chmod 755 "$stage/bin/mock"*
export PATH="$stage/bin:$PATH" FIXTURE_LOG="$stage/calls" old=upower new=uke-upower unit=upower.service
export FIXTURE_ENABLED=enabled FIXTURE_ACTIVE=active
phase=save sh "$stage/fragment"
test "$(stat -c %a "$stage/state/$new")" = 600
export FIXTURE_ENABLED=disabled FIXTURE_ACTIVE=inactive
phase=restore sh "$stage/fragment"
grep -Fx 'enable upower.service' "$stage/calls"
grep -Fx 'start upower.service' "$stage/calls"
test ! -e "$stage/state/$new"
: >"$stage/calls"
phase=save sh "$stage/fragment"
phase=restore sh "$stage/fragment"
! grep -Eq '^(enable|start) ' "$stage/calls"
: >"$stage/calls"
export FIXTURE_ENABLED=enabled FIXTURE_ACTIVE=active
phase=save sh "$stage/fragment"
export FIXTURE_ENABLED=masked
phase=restore sh "$stage/fragment"
! grep -Eq '^(enable|start) ' "$stage/calls"
export FIXTURE_ENABLED=enabled
phase=save sh "$stage/fragment"
export FIXTURE_START_FAIL=1
! phase=restore sh "$stage/fragment"
test -f "$stage/state/$new"
unlink "$stage/state/$new"
ln -s "$stage/calls" "$stage/state/$new"
! phase=save sh "$stage/fragment"
export new=uke-iio-sensor-proxy FIXTURE_SELINUX=0
phase=restore sh "$stage/fragment"
grep -Fx 'policy:-i /usr/share/selinux/packages/uke-ssc.cil' "$stage/calls"
printf '%s\n' 'PASS: admitted active/enabled state, disabled/masked refusal, retained failed restoration and symlink rejection; host stubs only.'
