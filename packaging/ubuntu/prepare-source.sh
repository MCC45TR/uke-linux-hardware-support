#!/bin/bash
# Assemble source only. Never execute upstream build rules or sign/upload.
set -euo pipefail
export LC_ALL=C
root=$(git -C "$(dirname "$0")" rev-parse --show-toplevel)
pkg=${1:?Usage: prepare-source.sh PACKAGE NEW_OUTPUT_DIRECTORY}
out=${2:?Specify a new output directory}
revision=cbecc441059ebffa97497e736edd2e0b0b0546a5
case "$pkg" in
 uke-battery-observer) component=uke-platform-runtime; version=1.2.0 ;;
 uke-front-color) component=uke-sensors; version=0.1.0 ;;
 *) echo 'No reviewed Debian source recipe for this package' >&2; exit 2 ;;
esac
[[ ! -e $out ]] || { echo 'Output directory must not exist' >&2; exit 2; }
mkdir -p "$out"
out=$(realpath "$out")
stage=$out/$pkg-$version
mkdir "$stage"
# git archive reads tracked source at an immutable commit; no donor scripts run.
paths=("$component/Makefile" "$component/LICENSE" "$component/src" "$component/packaging" "$component/manifests" "$component/docs" "$component/tests")
git -C "$root" archive "$revision" "${paths[@]}" | tar -x -C "$stage" --strip-components=1
printf '%s\n' "Repository: https://github.com/MCC45TR/uke-linux-hardware-support" "Revision: $revision" "Component: $component" > "$stage/SOURCE-ORIGIN"
epoch=$(git -C "$root" show -s --format=%ct "$revision")
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf "$out/${pkg}_${version}.orig.tar.xz" -C "$out" "$pkg-$version"
cp -a "$root/packaging/ubuntu/recipes/$pkg/debian" "$stage/debian"
(cd "$out"; dpkg-source -b "$pkg-$version")
(cd "$stage"; dpkg-genchanges -S -sa > "$out/${pkg}_$(dpkg-parsechangelog -S Version)_source.changes")
(cd "$out"; sha256sum ./*.dsc ./*.tar.xz ./*.changes > SHA256SUMS)
