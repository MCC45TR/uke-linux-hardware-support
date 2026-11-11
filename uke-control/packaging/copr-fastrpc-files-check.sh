#!/usr/bin/bash
# Source worker only. Preserve native99 and add original-library file API checks.
set -Eeuo pipefail
[[ ! -e /etc/senemos/kernel-update.conf && $# == 2 && $2 == 100 ]]
output=$(realpath -m "$1")
[[ ! -e $output && ! -L $output ]]
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
workspace=$(cd "$owner/.." && pwd)
contract=$owner/packaging/fastrpc-files.json
(cd "$workspace";jq -r '.source_assets[]|.sha256+"  "+.path' "$contract" | sha256sum -c --strict --quiet -)
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
mkdir "$output" "$work/upstream" "$work/bundle"
curl -qfsSL --retry 2 --max-time 90 "$(jq -er .parent.url "$contract")" -o "$work/parent.rpm"
printf '%s  %s\n' "$(jq -er .parent.srpm_sha256 "$contract")" "$work/parent.rpm" | sha256sum -c --strict --quiet -
rpm2cpio "$work/parent.rpm" | cpio -t --quiet | awk '/^\/|(^|\/)\.\.($|\/)/{bad=1}END{exit bad}'
rpm2cpio "$work/parent.rpm" | (cd "$output";cpio -idm --quiet --no-absolute-filenames)
[[ $(find "$output" -maxdepth 1 -type f | wc -l) == 2 ]]
printf '%s  %s\n' "$(jq -er .parent.source_archive_sha256 "$contract")" "$output/uke-fastrpc-notify-check-1.tar.xz" | sha256sum -c --strict --quiet -
curl -qfsSL --retry 2 --max-time 90 "$(jq -er .upstream.url "$contract")" -o "$work/upstream.tar.gz"
printf '%s  %s\n' "$(jq -er .upstream.archive_sha256 "$contract")" "$work/upstream.tar.gz" | sha256sum -c --strict --quiet -
tar -xzf "$work/upstream.tar.gz" -C "$work/upstream"
source=$work/upstream/fastrpc-1.0.8
printf '%s  %s\n' "$(jq -er .upstream.apps_std_header_sha256 "$contract")" "$source/inc/apps_std.h" | sha256sum -c --strict --quiet -
printf '%s  %s\n' "$(jq -er .upstream.license_sha256 "$contract")" "$source/LICENSE.txt" | sha256sum -c --strict --quiet -
bundle=$work/bundle/uke-fastrpc-files-1
mkdir "$bundle" "$bundle/upstream"
install -m644 "$source/inc/apps_std.h" "$bundle/upstream/apps_std.h"
install -m644 "$source/LICENSE.txt" "$bundle/LICENSE.fastrpc"
install -m644 "$owner/tests/fastrpc-files.cpp" "$bundle/fastrpc-files.cpp"
install -m644 "$owner/docs/FASTRPC-FILES.md" "$bundle/FASTRPC-FILES.md"
install -m644 "$contract" "$bundle/fastrpc-files.json"
commit=$(git -C "$owner" rev-parse HEAD)
tree=$(git -C "$owner" rev-parse HEAD^{tree})
jq -n --arg commit "$commit" --arg tree "$tree" '{source_commit:$commit,source_tree:$tree,native_revision:100,scope:"Original Fedora file APIs with ioctl syscalls denied;no DSP or provider transition",new_runtime_packages:0}' > "$bundle/files-source-receipt.json"
(cd "$bundle";find . -type f -print0 | sort -z | xargs -0 sha256sum > "$work/seal")
install -m644 "$work/seal" "$bundle/SHA256SUMS"
epoch=$(git -C "$owner" show -s --format=%ct HEAD)
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf "$output/uke-fastrpc-files-1.tar.xz" -C "$work/bundle" uke-fastrpc-files-1
node "$owner/packaging/fold-fastrpc-files-spec.mjs" "$output/uke-kernel-migration-check.spec" "$contract"
[[ $(rpmspec -q --srpm --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$output/uke-kernel-migration-check.spec") == uke-kernel-migration-check-7.3.3-100.fc46 ]]
printf 'Native100:exact native99 source plus original SDK header and file API fixture;no rebuilt Fedora library or runtime delivery.\n'
