#!/usr/bin/env bash
# Explicit future host packaging operation. No download, install or activation.
set -Eeuo pipefail
root=$(cd -- "$(dirname -- "$0")/../.." && pwd)
archive=$(realpath "${1:?Pass the pinned original upower-v1.91.5.tar.bz2}")
out=${2:?Pass a new empty output directory}
[[ ! -e $out ]] || { echo 'Output already exists' >&2; exit 1; }
digest=$(sha256sum -- "$archive"); digest=${digest%% *}
[[ $digest == c465a1c7fc05d00ac2bc7321c19b9b7e479410d8392bd7eb3e714275729629fd ]] || { echo 'UPower source identity mismatch' >&2; exit 1; }
mkdir -p "$out"/{BUILD,BUILDROOT,RPMS,SRPMS,SOURCES,SPECS}
out=$(realpath "$out")
cp -- "$archive" "$out/SOURCES/upower-v1.91.5.tar.bz2"
cp -- "$root"/patches/upower/*.patch "$root"/tests/upower-{threshold-contract,health-contract,fixture}.sh "$root/tests/fixtures/upower/ac-net-discharge.umockdev" "$out/SOURCES/"
cp -- "$root/packaging/upower/upower.spec" "$out/SPECS/"
cp -- "$root/packaging/upower/source-lock.json" "$out/"
rpmbuild -bs --nodeps --target aarch64 --define "_topdir $out" "$out/SPECS/upower.spec"
