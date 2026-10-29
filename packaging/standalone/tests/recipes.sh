#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Source RPM fixture: run in the COPR source worker, never on the tablet.
set -Eeuo pipefail
root=$(cd -- "$(dirname -- "$0")/../../.."; pwd)
cd "$root"
bash packaging/standalone/tests/manifest.sh
stage=$(mktemp -d)
trap 'rm -r --one-file-system -- "$stage"' EXIT
version=$(jq -er '.native_versions["uke-control"]' packaging/families/families.json)
part="$stage/parts/uke-control"
mkdir -p "$part" "$stage/uke-core-$version/inputs/uke-control" "$stage/output/"{SOURCES,SPECS,SRPMS}
: >"$stage/rename"
cat >"$stage/input.spec" <<EOF
Name: uke-control
Version: $version
Release: 1.fc46
Summary: Independent source fixture
License: MIT
BuildRequires: make
%description
Source fixture.
%package tests
Summary: Test child
Requires: uke-control = $version-1.fc46
%description tests
Test fixture.
%prep
cd /__UKE_BUILD__/uke-control
echo retained-prep
%build
: __UKE_CWD__=fixture
: __UKE_BUILDROOT__=/__UKE_ROOT__
: __UKE_BUILDDIR__=/__UKE_BUILD__/uke-control
echo retained-build
%install
echo retained-install /__UKE_ROOT__
%check
echo retained-check
%files
%license LICENSE
/usr/share/uke-fixture
%files tests
/usr/share/uke-fixture-test
EOF
awk -v out="$part" -v mapping="$stage/rename" -v native_version="$version" -v release_override=1.unstable1.fc46 -f packaging/families/host/split-spec.awk "$stage/input.spec"
printf '%s\n' 'fixture source, not an executable' >"$stage/uke-core-$version/inputs/uke-control/source.txt"
printf 'uke-control\tfixture\n' >"$stage/uke-core-$version/INPUT-SRPMS.tsv"
bash packaging/standalone/host/emit.sh uke-control uke-core "$stage" "$stage/output" aarch64 .fc46
spec="$stage/output/SPECS/uke-control.spec"
[[ $(rpmspec -q --srpm --qf '%{NAME}' "$spec") == uke-control ]]
cmp <(rpmspec -q --qf '%{NAME}\n' "$spec" | LC_ALL=C sort) <(printf '%s\n' uke-control uke-control-tests)
grep -Fxq "Requires: uke-control = $version-1.unstable1.fc46" "$spec"
grep -Fxq "Provides: uke-core = $version-1.fc46" "$spec"
grep -Fxq "Obsoletes: uke-core <= $version-1.fc46" "$spec"
! grep -q '/__UKE_' "$spec"
for phase in prep build install check; do grep -Fxq "echo retained-$phase" "$spec" || grep -q "^echo retained-$phase " "$spec"; done
test -s "$stage/output/SRPMS/uke-control-$version-1.unstable1.fc46.src.rpm"
printf '%s\n' 'PASS: independent source identity, exact child requirements, declared selection replacement, and retained build/check phases.'
