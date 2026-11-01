#!/bin/bash
# Host-only source extraction and shared patch application. No network or
# upstream build/test program runs here. Invoke from the recipe source root.
set -euo pipefail
export LC_ALL=C
archive=source/referances/libssc/libssc-v0.4.4.tar.gz
printf '%s  %s\n' 716d6bd6b34d2d753060c6b54c9a87e34fae75b724c763bf9ef487efa3621587 "$archive" | sha256sum -c -
[[ ! -e upstream ]]
work=$(mktemp -d .main-libssc.XXXXXX)
trap 'rm -rf -- "$work"' EXIT
tar -tf "$archive" > "$work/members"
! grep -Eq '(^/|(^|/)\.\.(/|$))' "$work/members"
mkdir "$work/tree"
tar -xf "$archive" -C "$work/tree" --strip-components=1
patches=(source/patches/libssc/*.patch)
[[ ${#patches[@]} == 7 ]]
for patch_file in "${patches[@]}"; do
 patch --batch --fuzz=0 --no-backup-if-mismatch -d "$work/tree" -p1 < "$patch_file"
done
mv "$work/tree" upstream
