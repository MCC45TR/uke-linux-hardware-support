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
grep -Fx 'Requires: libstdc++%{?_isa} >= 16.2.1' "$stage/output" >/dev/null
grep -Fx 'Conflicts: uke-libstdcxx <= 16.2.1-19.fc46' "$stage/output" >/dev/null
grep -Fx 'Provides: uke-core-meta(fedora-cxx-runtime-v1) = 1' "$stage/output" >/dev/null
grep -Fx 'Conflicts: uke-charge-control < 1.4.1-3.fc46' "$stage/output" >/dev/null
grep -Fx 'echo unchanged-fixture' "$stage/output" >/dev/null
if bash "$root/packaging/families/host/fedora-return-meta.sh" "$stage/output" "$stage/duplicate"; then exit 1; fi
echo 'PASS: bounded Fedora returns require compatible GNU consumers and preserve unrelated scriptlets.'
