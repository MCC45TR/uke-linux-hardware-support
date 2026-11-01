#!/bin/bash
# Assemble source only. Never execute upstream build rules or sign/upload.
set -euo pipefail
umask 022
export LC_ALL=C
root=$(git -C "$(dirname "$0")" rev-parse --show-toplevel)
pkg=${1:?Usage: prepare-source.sh PACKAGE NEW_OUTPUT_DIRECTORY}
out=${2:?Specify a new output directory}
revision=cbecc441059ebffa97497e736edd2e0b0b0546a5
[[ $pkg =~ ^[a-z0-9][a-z0-9+.-]+$ ]] || exit 2
row=$(awk -F '\t' -v package="$pkg" '$1 == package {print $2 "\t" $3}' "$root/packaging/ubuntu/components.tsv")
[[ -n $row && $row != *$'\n'* ]] || { echo 'No unique reviewed source recipe' >&2; exit 2; }
IFS=$'\t' read -r component version <<< "$row"
[[ ! -e $out ]] || { echo 'Output directory must not exist' >&2; exit 2; }
mkdir -p "$out"
out=$(realpath "$out")
stage=$out/$pkg-$version
mkdir "$stage"
# Include tracked sources and their licenses, never build outputs or private inputs.
paths=()
for item in Makefile LICENSE LICENSE.GPL-3.0 LICENSE.Apache-2.0 licenses src packaging manifests docs tests patches; do
 if git -C "$root" cat-file -e "$revision:$component/$item" 2>/dev/null; then paths+=("$component/$item"); fi
done
git -C "$root" archive "$revision" "${paths[@]}" | tar -x -C "$stage" --strip-components=1
source "$root/packaging/ubuntu/prepare-upstream.sh"
source "$root/packaging/ubuntu/prepare-release.sh"
if [[ $pkg == uke-touchscreen ]]; then source "$root/packaging/ubuntu/prepare-touch.sh"; fi
if [[ $pkg == uke-orangefox-recovery ]]; then source "$root/packaging/ubuntu/prepare-recovery.sh"; fi
printf '%s\n' "Repository: https://github.com/MCC45TR/uke-linux-hardware-support" "Revision: $revision" "Component: $component" > "$stage/SOURCE-ORIGIN"
epoch=$(git -C "$root" show -s --format=%ct "$revision")
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf "$out/${pkg}_${version}.orig.tar.xz" -C "$out" "$pkg-$version"
cp -a "$root/packaging/ubuntu/recipes/$pkg/debian" "$stage/debian"
find "$stage/debian" -type d -exec chmod 0755 {} +
find "$stage/debian" -type f -perm /111 -exec chmod 0755 {} +
find "$stage/debian" -type f ! -perm /111 -exec chmod 0644 {} +
if [[ -n ${upstream:-} ]]; then
 mkdir -p "$stage/debian/patches"
 cp "$stage/uke/patches/$patchset/"*.patch "$stage/debian/patches/"
 (cd "$stage/debian/patches"; printf "%s\n" *.patch > series)
fi
(cd "$out"; dpkg-source -b "$pkg-$version")
(cd "$stage"; dpkg-genchanges -S -sa > "$out/${pkg}_$(dpkg-parsechangelog -S Version)_source.changes")
(cd "$out"; sha256sum ./*.dsc ./*.tar.xz ./*.changes > SHA256SUMS)
