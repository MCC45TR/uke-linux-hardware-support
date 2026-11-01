#!/bin/bash
# Official host tools only; no fixture or upstream build rules are executed.
set -euo pipefail
umask 022
root=$(git -C "$(dirname "$0")" rev-parse --show-toplevel)
scratch=$(mktemp -d)
trap 'rm -rf "$scratch"' EXIT
git init -q --bare "$scratch/hub.git"
metadata=
for suffix in a b; do
 package=uke-recipe-fixture-$suffix
 stage=$scratch/$package-0.1.0
 mkdir -p "$stage"
 printf 'Distinct source payload %s\n' "$suffix" > "$stage/README"
 tar -cJf "$scratch/${package}_0.1.0.orig.tar.xz" -C "$scratch" "$package-0.1.0"
 mkdir -p "$stage/debian/source"
 echo '3.0 (quilt)' > "$stage/debian/source/format"
 printf '%s\n' "Source: $package" 'Section: misc' 'Priority: optional' 'Maintainer: Test <test@example.invalid>' 'Standards-Version: 4.7.0' '' "Package: $package" 'Architecture: arm64' 'Description: source archive identity fixture' > "$stage/debian/control"
 printf '%s\n' "$package (0.1.0-1~tenacious1) tenacious; urgency=low" '' '  * Fixture only.' '' ' -- Test <test@example.invalid>  Sun, 01 Nov 2026 00:00:00 +0000' > "$stage/debian/changelog"
 printf '#!/usr/bin/make -f\n%%:\n\ttrue\n' > "$stage/debian/rules"
 chmod 755 "$stage/debian/rules"
 (cd "$scratch"; dpkg-source -b "$package-0.1.0") >/dev/null
 export_tree=$scratch/export-$suffix
 bash "$root/packaging/ubuntu/prepare-recipe-source.sh" "$scratch/${package}_0.1.0-1~tenacious1.dsc" "$export_tree" "$metadata" >/dev/null
 git -C "$export_tree" push -q "$scratch/hub.git" "launchpad/$package" pristine-tar
 metadata=$export_tree
done
# A generic version tag is deliberately ambiguous. Exact pristine metadata
# must win even when both unrelated packages share the same upstream version.
git -C "$scratch/export-a" tag upstream/0.1.0 source/uke-recipe-fixture-a/0.1.0
git -C "$scratch/export-a" push -q "$scratch/hub.git" upstream/0.1.0
for suffix in a b; do
 package=uke-recipe-fixture-$suffix
 printf '%s\n' '# git-build-recipe format 0.4 deb-version {debversion}+git{revtime}' "$scratch/hub.git launchpad/$package" > "$scratch/$package.recipe"
 git-build-recipe --safe --no-build --distribution tenacious "$scratch/$package.recipe" "$scratch/result-$suffix" >/dev/null 2>&1
 cmp "$scratch/${package}_0.1.0.orig.tar.xz" "$scratch/result-$suffix/${package}_0.1.0.orig.tar.xz"
 (cd "$scratch/result-$suffix"; dpkg-source -b "$package") >/dev/null
done
echo 'Colliding-version recipe fixtures reconstructed both exact source archives.'
