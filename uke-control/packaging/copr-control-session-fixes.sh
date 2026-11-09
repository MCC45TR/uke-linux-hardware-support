#!/usr/bin/bash
# Source worker only: existing Control/TUI/SDK outputs, no new runtime service.
set -Eeuo pipefail
test ! -e /etc/senemos/kernel-update.conf
[[ $# == 1 ]]
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
output=$(realpath -m "${1:?COPR source result}")
bash "$owner/packaging/copr-control-speaker-release.sh" "$output"
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
for path in packaging/accept-snapshot-identity.patch packaging/apply-session-fixes.sh \
 src/brightness-curve.cpp src/brightness-curve-config.hpp tests/brightness-curve.cpp \
 packaging/30-uke-brightness-curve.sh docs/BRIGHTNESS-CURVE.md docs/SESSION-ACCEPTANCE-FIXES.md;do
 install -Dm644 "$owner/$path" "$work/overlay/$path"
done
tar --sort=name --mtime=@1794253000 --owner=0 --group=0 --numeric-owner -cJf "$output/uke-control-session-fixes.tar.xz" -C "$work/overlay" .
install -m644 "$owner/packaging/copr-control-session-fixes.sh" "$output/"
bash "$owner/packaging/fold-session-fixes-spec.sh" "$output/uke-control.spec"
[[ $(grep -Fc 'application_version="2.14.0"' "$output/control-version.hpp") == 1 ]]
sed -i 's/application_version="2.14.0"/application_version="2.15.0"/' "$output/control-version.hpp"
[[ $(rpmspec -q --srpm --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$output/uke-control.spec") == uke-control-2.15.0-1.fc46 ]]
rpmspec -q --qf '%{NAME}\n' "$output/uke-control.spec" | sort > "$work/names"
printf '%s\n' uke-control uke-control-client-devel uke-control-tui > "$work/expected"
cmp "$work/names" "$work/expected"
bash "$owner/tests/kernel-consolidation.sh" "$output/uke-control.spec"
bash "$owner/tests/speaker-state.sh" "$output/uke-control.spec"
printf 'Control215 source prepared over exact self-contained214; two session fixes, three existing RPM outputs, no new service.\n'
