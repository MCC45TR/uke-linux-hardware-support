#!/usr/bin/env bash
set -Eeuo pipefail
stage=$(mktemp -d)
trap 'rm -rf -- "$stage"' EXIT
touch "$stage/rename"
for version in 1.0.19 0.1.0; do
    mkdir "$stage/$version"
    cat >"$stage/input" <<SPEC
Name: uke-control
Version: $version
Release: 2.fc46
Summary: Fixture
License: MIT
%description
Fixture.
%build
: __UKE_CWD__=fixture
: __UKE_BUILDROOT__=/buildroot
: __UKE_BUILDDIR__=/builddir
%files
SPEC
    awk -v out="$stage/$version" -v mapping="$stage/rename" -v native_version=1.0.19 -f host/split-spec.awk "$stage/input"
done

# Previously composed sources already contain explicit child versions and licenses.
mkdir "$stage/integration"
cat >"$stage/integration-input" <<'SPEC'
Name: uke-fixture
Version: 2.0
Release: 7.fc46
Epoch: 1
Summary: Integration fixture
License: MIT
Obsoletes: uke-legacy < 2.0-7.fc46
%description
Fixture.
%package -n uke-fixture-child
Version: 1.4
Release: 3.fc46
License: GPL-3.0-or-later
Summary: Independent child
Requires: uke-fixture(aarch-64) = 2.0-7.fc46
%description -n uke-fixture-child
Child.
%build
: __UKE_CWD__=fixture
: __UKE_BUILDROOT__=/buildroot
: __UKE_BUILDDIR__=/builddir
%files
%files -n uke-fixture-child
SPEC
awk -v preserve_identity=1 -v out="$stage/integration" -v mapping="$stage/rename" -f host/split-spec.awk "$stage/integration-input"
[[ $(grep -c '^Version:' "$stage/integration/preamble") == 2 ]]
grep -Fx 'Version: 2.0' "$stage/integration/preamble"
grep -Fx 'Version: 1.4' "$stage/integration/preamble"
grep -Fx 'Release: 8.fc46' "$stage/integration/preamble"
grep -Fx 'Release: 4.fc46' "$stage/integration/preamble"
grep -Fx 'Requires: uke-fixture(aarch-64) = 2.0-8.fc46' "$stage/integration/preamble"
grep -Fx 'Obsoletes: uke-legacy < 2.0-7.fc46' "$stage/integration/preamble"
[[ $(grep -c '^Epoch: 1$' "$stage/integration/preamble") == 2 ]]
[[ $(grep -c '^License:' "$stage/integration/preamble") == 2 ]]
printf '%s\n' 'PASS: integration identities, per-child EVRs, licenses, exact dependencies and compatibility bounds are preserved.'
grep -Fx 'Release: 2.fc46' "$stage/1.0.19/preamble"
grep -Fx 'Release: 1.fc46' "$stage/0.1.0/preamble"
mkdir "$stage/material"
printf 'material-decoration\tuke-material-decoration\n' >"$stage/rename"
sed 's/Name: uke-control/Name: material-decoration/' "$stage/input" >"$stage/material-input"
awk -v out="$stage/material" -v mapping="$stage/rename" -f host/split-spec.awk "$stage/material-input"
grep -Fx '%package -n uke-material-decoration' "$stage/material/preamble"
grep -Fx 'Provides: material-decoration = 0.1.0-3.fc46' "$stage/material/preamble"
grep -Fx 'Obsoletes: material-decoration < 0.1.0-3.fc46' "$stage/material/preamble"
grep -Fx 'Conflicts: material-decoration' "$stage/material/preamble"
mkdir "$stage/standalone"
awk -v standalone=material-decoration -v out="$stage/standalone" -v mapping="$stage/rename" -f host/split-spec.awk "$stage/material-input"
grep -Fx 'Name: uke-material-decoration' "$stage/standalone/preamble"
! grep -q '^%package' "$stage/standalone/preamble"
grep -Fx '%files -n uke-material-decoration' "$stage/standalone/tail"
grep -Fx $'uke-material-decoration\t0.1.0\t3.fc46' "$stage/standalone/standalone-identity"
mkdir "$stage/sensor"
sed 's/Name: uke-control/Name: iio-sensor-proxy/; /Summary: Fixture/aProvides: iio-sensor-proxy(ssc) = 1\nProvides: iio-sensor-proxy(uke-dual-light) = 1' "$stage/input" >"$stage/sensor-input"
awk -v out="$stage/sensor" -v mapping="$stage/rename" -f host/split-spec.awk "$stage/sensor-input"
for capability in ssc uke-dual-light; do
    grep -Fxq "Provides: iio-sensor-proxy($capability) = 1" "$stage/sensor/preamble"
    grep -Fxq "Provides: uke-iio-sensor-proxy($capability) = 1" "$stage/sensor/preamble"
done
printf '%s\n' 'PASS: native version conversion resets Release once; later packaging revisions are retained.'
mkdir "$stage/licenses"
awk '/^%build/ {print "%package docs\nSummary: Documentation fixture\nLicense: GFDL-1.1-or-later\n%description docs\nDocumentation."} {print}' "$stage/input" >"$stage/licenses-input"
awk -v out="$stage/licenses" -v mapping="$stage/rename" -f host/split-spec.awk "$stage/licenses-input"
[[ $(grep -c '^License:' "$stage/licenses/preamble") == 2 ]]
grep -Fx 'License: MIT' "$stage/licenses/preamble"
grep -Fx 'License: GFDL-1.1-or-later' "$stage/licenses/preamble"
printf '%s\n' 'PASS: explicit subpackage licenses are retained once without inheriting the parent license.'
mkdir "$stage/missing-macro"
sed '/^%build/a%cargo_prep' "$stage/input" >"$stage/missing-macro-input"
if awk -v out="$stage/missing-macro" -v mapping="$stage/rename" -f host/split-spec.awk "$stage/missing-macro-input" >"$stage/missing-macro.log" 2>&1; then
    echo 'Unexpanded build macro was accepted' >&2; exit 1
fi
grep -Fq 'Unexpanded build macro' "$stage/missing-macro.log"
printf '%s\n' 'PASS: missing source-host RPM macros fail before a family SRPM is emitted.'

# A release-specific source worker keeps every generated child in its target release.
for fedora in 44 45 46; do
    mkdir "$stage/fedora-$fedora"
    awk -v dist=".fc$fedora" -v out="$stage/fedora-$fedora" -v mapping="$stage/rename" -v native_version=9.0.0 -f host/split-spec.awk "$stage/input"
    grep -Fx "Release: 1.fc$fedora" "$stage/fedora-$fedora/preamble"
done
