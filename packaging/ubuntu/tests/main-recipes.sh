#!/bin/bash
# Static metadata/data tests only; no upstream compiler, helper or device runs.
set -euo pipefail
root=$(git -C "$(dirname -- "$0")" rev-parse --show-toplevel)
work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT
package=uke-speaker-firmware
bash "$root/packaging/ubuntu/prepare-main-overlay.sh" "$package" "$work/overlay"
diff -ru "$work/overlay" "$root/packaging/ubuntu/main-recipes/$package/debian"
inputs=$root/xiaomi-uke-firmware/referances/speaker-preset-3.0.304.0
metadata=$root/xiaomi-uke-firmware/packaging/speaker-firmware
bash "$root/xiaomi-uke-firmware/src/audit-speaker-source.sh" "$inputs/fs19xx.fsm" "$inputs/odm-NOTICE.xml.gz" "$metadata"
mkdir "$work/tree" "$work/tree/debian"
cp "$work/overlay/verify-main-inputs" "$work/tree/debian/"
revision=0123456789012345678901234567890123456789
for name in source metadata packaging; do
 printf 'nest-part %s fixture %s %s git-commit:%s\n' "$name" "$name" "$name" "$revision"
done > "$work/tree/debian/git-build-recipe.manifest"
(cd "$work/tree"; bash debian/verify-main-inputs)
sed -i 's/git-commit:0/git-commit:1/' "$work/tree/debian/git-build-recipe.manifest"
# Only one input is changed; all-three changes would still represent one commit.
sed -i '2s/git-commit:1/git-commit:2/' "$work/tree/debian/git-build-recipe.manifest"
if (cd "$work/tree"; bash debian/verify-main-inputs) >/dev/null 2>&1; then
 echo 'Mixed main revisions were admitted' >&2; exit 1
fi
cp "$inputs/fs19xx.fsm" "$work/changed.fsm"
echo corruption >> "$work/changed.fsm"
if bash "$root/xiaomi-uke-firmware/src/audit-speaker-source.sh" "$work/changed.fsm" "$inputs/odm-NOTICE.xml.gz" "$metadata" >/dev/null 2>&1; then
 echo 'Changed canonical preset was admitted' >&2; exit 1
fi
version=$(dpkg-parsechangelog --file "$work/overlay/changelog" -SVersion)
grep -Fxq "# git-build-recipe format 0.4 deb-version $version+git{revtime:source}" "$root/packaging/ubuntu/launchpad-main-recipes/$package.recipe"
grep -Fxq '3.0 (native)' "$work/overlay/source/format"
echo 'Canonical source, generated overlay and same-main revision refusals passed.'
