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
grep -Fx 'Release: 2.fc46' "$stage/1.0.19/preamble"
grep -Fx 'Release: 1.fc46' "$stage/0.1.0/preamble"
mkdir "$stage/material"
printf 'material-decoration\tmaterial-decoration\n' >"$stage/rename"
sed 's/Name: uke-control/Name: material-decoration/' "$stage/input" >"$stage/material-input"
awk -v out="$stage/material" -v mapping="$stage/rename" -f host/split-spec.awk "$stage/material-input"
grep -Fx '%package -n material-decoration' "$stage/material/preamble"
! grep -q '^Conflicts: material-decoration' "$stage/material/preamble"
printf '%s\n' 'PASS: native version conversion resets Release once; later packaging revisions are retained.'
