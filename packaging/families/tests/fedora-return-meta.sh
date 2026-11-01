#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
set -Eeuo pipefail
root=$(cd -- "$(dirname -- "$0")/../../.."; pwd)
stage=$(mktemp -d)
trap 'rm -rf -- "$stage"' EXIT
cat > "$stage/input" <<'SPEC'
Name: uke-core-meta
Requires: bluez libinput uke-libwacom uke-libssc
Requires: senemos-native-runtime(libstdc++)
%description
Source fixture.
%post
echo unchanged-fixture
SPEC
bash "$root/packaging/families/host/fedora-return-meta.sh" "$stage/input" "$stage/output"
grep -Eq '^Requires: bluez libinput libwacom >= 2.20.0 +uke-libssc$' "$stage/output"
grep -Fx 'Requires: senemos-native-runtime(libstdc++)' "$stage/output" >/dev/null
! grep -Fq 'Conflicts: uke-libstdcxx' "$stage/output"
grep -Fx 'echo unchanged-fixture' "$stage/output" >/dev/null
if bash "$root/packaging/families/host/fedora-return-meta.sh" "$stage/output" "$stage/duplicate"; then exit 1; fi
echo 'PASS: bounded desktop return metadata retains the console GNU policy and unrelated scriptlets.'
