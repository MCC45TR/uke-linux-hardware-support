#!/usr/bin/bash
# SPDX-License-Identifier: MIT
# COPR source worker; preserve the actual accepted daemon source and five patches.
set -Eeuo pipefail
[[ $# == 1 && ! -e /etc/senemos/kernel-update.conf ]]
component=$(cd -- "$(dirname -- "$0")/../.." && pwd)
workspace=$(dirname "$component")
contract=$component/packaging/upower/fedora-libs.json
output=$(realpath -m "$1")
[[ ! -e $output && ! -L $output && -z $(git -C "$workspace" status --porcelain) ]]
(cd "$workspace";jq -r '.source_assets[]|.sha256+"  "+.path' "$contract" | sha256sum -c --strict --quiet -)
work=$(mktemp -d);trap 'find "$work" -depth -delete' EXIT
mkdir "$output" "$work/overlay"
curl -qfsSL --retry 2 --max-time 120 "$(jq -er .parent.url "$contract")" -o "$work/parent.rpm"
printf '%s  %s\n' "$(jq -er .parent.srpm_sha256 "$contract")" "$work/parent.rpm" | sha256sum -c --strict --quiet -
rpm2cpio "$work/parent.rpm" | cpio -t --quiet | awk '/^\/|(^|\/)\.\.($|\/)/{bad=1}END{exit bad}'
rpm2cpio "$work/parent.rpm" | (cd "$output";cpio -idm --quiet --no-absolute-filenames)
[[ $(find "$output" -maxdepth 1 -type f ! -name uke-upower.spec | wc -l) == 1 ]]
printf '%s  %s\n' "$(jq -er .parent.archive_sha256 "$contract")" "$output/uke-upower-1.91.5.tar.xz" | sha256sum -c --strict --quiet -
for file in check-stock-library.sh retire-built-library-files.sh;do install -m644 "$component/packaging/upower/$file" "$work/overlay/$file";done
install -m644 "$component/packaging/upower/FEDORA-LIBRARY-RETURN.md" "$work/overlay/FEDORA-LIBRARY-RETURN.md"
jq --arg source "$(git -C "$workspace" rev-parse HEAD)" --arg tree "$(git -C "$workspace" rev-parse HEAD^{tree})" '.+{source_commit:$source,source_tree:$tree}' "$contract" > "$work/overlay/fedora-libs.json"
(cd "$work/overlay";find . -type f ! -name SHA256SUMS -print0 | sort -z | xargs -0 sha256sum > SHA256SUMS)
epoch=$(git -C "$workspace" show -s --format=%ct HEAD)
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf "$output/uke-upower-fedora-libs.tar.xz" -C "$work/overlay" .
node "$component/packaging/upower/fold-fedora-libs-spec.mjs" "$output/uke-upower.spec" "$contract"
printf '%s  %s\n' "$(jq -er .result.spec_sha256 "$contract")" "$output/uke-upower.spec" | sha256sum -c --strict --quiet -
[[ $(rpmspec --define "_sourcedir $output" -q --srpm --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$output/uke-upower.spec") == uke-upower-1.91.5-21.fc46 ]]
rpmspec --define "_sourcedir $output" -q --qf '%{NAME}\n' "$output/uke-upower.spec" | sort > "$work/names"
printf '%s\n' uke-upower uke-upower-devel uke-upower-devel-docs > "$work/expected"
cmp "$work/names" "$work/expected"
printf 'UPower21: actual20 source archive unchanged, necessary daemon fixes retained, original Fedora library and two existing metadata-only development bridges; no library RPM.\n'
