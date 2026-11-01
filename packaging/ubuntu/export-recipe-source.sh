#!/bin/bash
# Export a prepared source package into a new, reviewable recipe Git checkout.
# Publication is a separate explicit git push; source bytes remain shared.
set -euo pipefail
stage=$(realpath "${1:?prepared source directory required}")
destination=${2:?new checkout directory required}
[[ -f $stage/debian/control && ! -e $destination ]]
package=$(sed -n 's/^Source: //p' "$stage/debian/control")
# Admit the first independently distributable route only. Expand after validation.
[[ $package == uke-speaker-firmware ]]
version=$(cd "$stage"; dpkg-parsechangelog -S Version)
upstream=${version%-*}
mkdir -p "$destination"
cp -a "$stage/." "$destination/"
[[ ! -e $destination/.git ]]
# dpkg-source creates quilt bookkeeping even when this route has no patches.
# It is extraction state, never part of the common source or recipe branch.
rm -rf -- "$destination/.pc"
mv "$destination/debian" "$destination.debian"
git -C "$destination" init -q --initial-branch="launchpad/$package"
git -C "$destination" config user.name MCC45TR
git -C "$destination" config user.email 75160848+MCC45TR@users.noreply.github.com
git -C "$destination" add .
git -C "$destination" commit -qm "Import exact shared $package $upstream source and attribution"
git -C "$destination" tag "upstream/$upstream"
mv "$destination.debian" "$destination/debian"
git -C "$destination" add debian
git -C "$destination" commit -qm "Add native Debian packaging for Tenacious ARM64 recipes"
git -C "$destination" diff --exit-code "upstream/$upstream" -- . ':!debian'
printf 'Exported %s; publish branch launchpad/%s and tag upstream/%s after validation.\n' "$destination" "$package" "$upstream"
