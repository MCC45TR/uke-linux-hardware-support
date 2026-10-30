#!/usr/bin/env bash
# Regular files only. No loops, mounts, chroot or target software execution.
set -euo pipefail
cd "$(dirname "$0")/.."
work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT
image=$work/filesystem.img
truncate -s 128M "$image"
[[ -f $image && ! -b $image && ! -L $image ]]
build/test-chroot-probe "$image" absent
for filesystem in ext4 f2fs; do
    truncate -s 0 "$image"
    truncate -s 128M "$image"
    if [[ $filesystem == ext4 ]]; then mkfs.ext4 -F -q "$image"; else mkfs.f2fs -f "$image" > "$work/mkfs.log"; fi
    sha256sum "$image" > "$work/before"
    build/test-chroot-probe "$image" "$filesystem"
    sha256sum "$image" > "$work/after"
    cmp "$work/before" "$work/after"
done
printf '%s\n' 'Disposable ext4/F2FS image probe preserved exact bytes; no block devices opened'
