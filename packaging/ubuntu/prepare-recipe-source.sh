#!/bin/bash
# Prepare a package-specific source view and exact orig metadata, without push.
# Requires official host dpkg-dev, Git and pristine-tar; runs no upstream code.
set -euo pipefail
umask 022
dsc=$(realpath "${1:?source DSC required}")
destination=${2:?new checkout directory required}
metadata=${3:-}
[[ -f $dsc && ! -e $destination ]]
dpkg-source --skip-patches -x "$dsc" "$destination"
destination=$(realpath "$destination")
package=$(sed -n 's/^Source: //p' "$destination/debian/control")
[[ $package =~ ^[a-z0-9][a-z0-9+.-]+$ ]]
version=$(cd "$destination"; dpkg-parsechangelog -S Version)
upstream=${version%-*}
tag=source/$package/${upstream//\~/_}
orig=$(dirname "$dsc")/${package}_${upstream}.orig.tar.xz
[[ -f $orig && ! -e $destination/.git ]]
rm -rf -- "$destination/.pc"
mv "$destination/debian" "$destination.debian"
git -C "$destination" init -q --initial-branch="launchpad/$package"
git -C "$destination" config user.name MCC45TR
git -C "$destination" config user.email 75160848+MCC45TR@users.noreply.github.com
git -C "$destination" add -f .
git -C "$destination" commit -qm "Import exact shared $package $upstream source"
# Package-specific tags avoid collisions between unrelated 0.1.0 components.
git -C "$destination" tag "$tag"
if [[ -n $metadata ]]; then
 git -C "$destination" fetch -q "$metadata" refs/heads/pristine-tar:refs/heads/pristine-tar
fi
(cd "$destination"; pristine-tar commit "$orig" "refs/tags/$tag")
mv "$destination.debian" "$destination/debian"
git -C "$destination" add -f debian
git -C "$destination" commit -qm "Package shared $package sources for Tenacious ARM64"
git -C "$destination" diff --exit-code "$tag" -- . ':!debian'
verification=$(mktemp -d)
trap 'rm -rf "$verification"' EXIT
(cd "$destination"; pristine-tar checkout "$verification/$(basename "$orig")")
cmp "$orig" "$verification/$(basename "$orig")"
printf '%s\t%s\t%s\n' "$package" "$(git -C "$destination" rev-parse HEAD)" "$(sha256sum "$orig" | cut -d ' ' -f1)"
