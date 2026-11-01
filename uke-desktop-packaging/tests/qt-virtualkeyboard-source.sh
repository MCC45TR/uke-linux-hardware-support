#!/bin/bash
# Check the real Fedora-spec transformation without executing upstream code.
set -Eeuo pipefail
project=$(cd -- "$(dirname -- "$0")/.."; pwd)
original=${1:?original Fedora specification}
stage=$(mktemp -d)
trap 'find "$stage" -depth -delete' EXIT
transform=$project/src/native-runtime/qt-virtualkeyboard-sound.awk
release=$(jq -er .release "$project/manifests/qt-virtualkeyboard-sound.json")
awk -v release="$release" -f "$transform" "$original" > "$stage/updated.spec"
grep -Fxq "Release: $release%{?dist}" "$stage/updated.spec"
grep -Fxq 'BuildRequires: qt6-qtmultimedia-devel = %{version}' "$stage/updated.spec"
grep -Fq -- '-DFEATURE_vkb_sound_effects:BOOL=ON' "$stage/updated.spec"
cmp <(sed -n '/^%prep$/,/^%build$/p' "$original") <(sed -n '/^%prep$/,/^%build$/p' "$stage/updated.spec")
cmp <(sed -n '/^%files devel/,$p' "$original" | sed '/^%changelog/,$d') <(sed -n '/^%files devel/,$p' "$stage/updated.spec" | sed '/^%changelog/,$d')
original_history=$(sed -n '/^%changelog/,$p' "$original" | sed '1d')
updated_history=$(sed -n '/^%changelog/,$p' "$stage/updated.spec" | sed '1,5d')
[[ $original_history == "$updated_history" ]]
for broken in feature dependency duplicate; do
    case $broken in
        feature) sed 's/^%cmake_qt6/%unexpected_qt6/' "$original" > "$stage/broken.spec";;
        dependency) sed '/^BuildRequires: qt6-qtdeclarative-devel/d' "$original" > "$stage/broken.spec";;
        duplicate) { cat "$original"; printf '\n%%files\n'; } > "$stage/broken.spec";;
    esac
    if awk -v release="$release" -f "$transform" "$stage/broken.spec" > "$stage/rejected.spec"; then exit 1; fi
done
printf 'Original Qt source/prep, devel/examples and changelog preserved; sound dependency/feature enabled; missing and duplicate source anchors refused\n'
