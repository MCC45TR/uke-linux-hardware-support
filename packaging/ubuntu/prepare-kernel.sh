#!/bin/bash
# Reproduce only the source composition of COPR release 115. Never run its
# worker, native prepare scripts, compiler tests, kernel build or image tools.
archive=$root/uke-kernel-packaging/referances/kernel-parent110.src.rpm
printf '%s  %s\n' 84a612c12c4b99ce29077273cdca6dce704fd94c364f4c7c50bcdeb9313af69c "$archive" | sha256sum -c -
mkdir "$stage/inputs"
for member in linux-7.3.1.tar.xz linux-7.3.1.tar.sign linux-stable.asc senemos-uke-adaptation-7.3.1.tar.xz senemos-native-profile-7.3.1.tar.xz senemos-uke-boot-recipe.tar.xz; do
 bsdtar -xOf "$archive" "$member" > "$stage/inputs/$member"
done
printf '%s  %s\n' "$(jq -er .source_sha256 "$stage/manifests/linux-7.3.1.json")" "$stage/inputs/linux-7.3.1.tar.xz" | sha256sum -c -
for member in senemos-uke-adaptation-7.3.1.tar.xz senemos-native-profile-7.3.1.tar.xz senemos-uke-boot-recipe.tar.xz; do
 tar -tf "$stage/inputs/$member" > "$out/kernel-members"
 ! grep -Eq '(^/|(^|/)\.\.(/|$))' "$out/kernel-members"
 tar -xf "$stage/inputs/$member" -C "$stage/inputs"
done
profile=$stage/inputs/native-profile
boot=$stage/inputs/boot-recipe
(cd "$profile"; sha256sum --check --strict --quiet SHA256SUMS)
grep -Fxq 1ea50711f0acf2e5cb3aa1cb7c349ba065c3db33 "$profile/source-commit"
printf '%s  %s\n' 8cdef42c010304bd948cf1fa83015c12b8cce637f647bfcf166de6c49efc3db1 "$profile/prepare.sh" | sha256sum -c -
cp "$profile/source-composition.json" "$profile/source-composition-parent731110.json"
overlays=(packaging/native/prepare.sh patches/boot/0057-llcc-optional-window.patch patches/boot/0058-fs19xx-current-workqueue.patch patches/boot/0059-lpass-sort-reg-defaults.patch tests/llcc-sm7675-contract.sh tests/audio-fs19xx-lifecycle-contract.sh src/boot/build-payload.sh src/boot/make-initramfs-cpio.sh tests/initramfs-cpio-contract.sh)
: > "$out/kernel-overlays"
for relative in "${overlays[@]}"; do
 destination=$relative
 [[ $relative != packaging/native/prepare.sh ]] || destination=prepare.sh
 install -Dm644 "$stage/$relative" "$profile/$destination"
 install -Dm644 "$stage/$relative" "$boot/$relative"
 printf '%s\n' "$destination" >> "$out/kernel-overlays"
done
# Verify every retained parent file, rather than assuming its tar was unchanged.
while read -r expected relative; do
 relative=${relative#./}
 case $relative in source-commit|source-composition.json) continue;; esac
 if ! grep -Fxq "$relative" "$out/kernel-overlays"; then
  printf '%s  %s\n' "$expected" "$profile/$relative" | sha256sum -c --status
 fi
done < "$profile/SHA256SUMS"
printf '%s\n' "$revision" > "$profile/source-commit"
jq -Rn '[inputs]' < "$out/kernel-overlays" > "$out/kernel-overlays.json"
jq --arg commit "$revision" --slurpfile overlays "$out/kernel-overlays.json" '. + {source_mode:"qualified-parent-with-bounded-boot-warning-fixes",source_commit:$commit,parent_copr_build:11239296,parent_srpm_sha256:"84a612c12c4b99ce29077273cdca6dce704fd94c364f4c7c50bcdeb9313af69c",parent_composition:"source-composition-parent731110.json",overlays:$overlays[0],adaptation_patches_unchanged:true,hardware_settings_unchanged:true,physical_acceptance:false}' "$profile/source-composition-parent731110.json" > "$profile/source-composition.json"
cp "$stage/manifests/boot-payload.json" "$boot/lock.json"
(cd "$profile"; find . -type f ! -name SHA256SUMS -print0 | sort -z | xargs -0 sha256sum > SHA256SUMS)
(cd "$boot"; jq -r '.source_assets[]|.sha256+"  "+.path' lock.json | sha256sum -c --strict --quiet)
# Keep only the composed trees; stale parent archives must not be built by mistake.
rm "$stage/inputs/senemos-native-profile-7.3.1.tar.xz" "$stage/inputs/senemos-uke-boot-recipe.tar.xz" "$stage/inputs/senemos-uke-adaptation-7.3.1.tar.xz"
printf '%s\n' 'Fedora source composition: 7.3.1-115' 'Parent source RPM SHA256: 84a612c12c4b99ce29077273cdca6dce704fd94c364f4c7c50bcdeb9313af69c' 'Source preparation only; native preparation tests and builds not executed.' > "$stage/KERNEL-SOURCE-ORIGIN"
