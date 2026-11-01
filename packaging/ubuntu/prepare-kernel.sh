#!/bin/bash
# Stage exact emitted experimental release-1006 sources only. Never execute its
# native preparer, tests, compiler, RPM worker or boot/image producers here.
archive=$root/uke-kernel-packaging/referances/source-reconciliation/kernel-1006.src.rpm
printf '%s  %s\n' 70f151855f727c9d83c3bedc35ff52dd60cb222b2d741818472dced458bcd60c "$archive" | sha256sum -c -
mkdir "$stage/inputs"
for member in linux-7.3.1.tar.xz linux-7.3.1.tar.sign linux-stable.asc senemos-uke-adaptation-7.3.1.tar.xz senemos-native-profile-7.3.1.tar.xz senemos-uke-boot-recipe.tar.xz; do
 bsdtar -xOf "$archive" "$member" > "$stage/inputs/$member"
done
printf '%s  %s\n' "$(jq -er .source_sha256 "$stage/manifests/linux-7.3.1.json")" "$stage/inputs/linux-7.3.1.tar.xz" | sha256sum -c -
# GitHub rejects individual files over 100 MiB. Preserve the signed Linux
# archive bytes as bounded source chunks rather than introducing Git LFS or
# downloading unpinned inputs from Launchpad's offline binary build.
split -b 80M -d -a 2 "$stage/inputs/linux-7.3.1.tar.xz" "$stage/inputs/linux-7.3.1.tar.xz.part-"
(cd "$stage/inputs"; sha256sum linux-7.3.1.tar.xz.part-* > linux-source-parts.sha256)
cmp "$stage/inputs/linux-7.3.1.tar.xz" <(cat "$stage/inputs/"linux-7.3.1.tar.xz.part-*)
rm "$stage/inputs/linux-7.3.1.tar.xz"
printf '%s  %s\n' 5fd98e97456a64f6ac65645b72187dce63c7dae125374cbb9dedb44cd1770bea "$stage/inputs/senemos-uke-adaptation-7.3.1.tar.xz" 5db7d0e8e3bd82107671667296fdd7d183de0f4cb4ec9daf746677f30329d84b "$stage/inputs/senemos-native-profile-7.3.1.tar.xz" d83d5a99b343844b2b85a3656e5eaf9e9103231dfa8dc192cd0d24679138846f "$stage/inputs/senemos-uke-boot-recipe.tar.xz" | sha256sum -c -
for member in senemos-uke-adaptation-7.3.1.tar.xz senemos-native-profile-7.3.1.tar.xz senemos-uke-boot-recipe.tar.xz; do
 tar -tf "$stage/inputs/$member" > "$out/kernel-members"
 ! grep -Eq '(^/|(^|/)\.\.(/|$))' "$out/kernel-members"
 tar -xf "$stage/inputs/$member" -C "$stage/inputs"
done
profile=$stage/inputs/native-profile
boot=$stage/inputs/boot-recipe
(cd "$profile"; sha256sum --check --strict --quiet SHA256SUMS)
grep -Fxq fcbdae4b757732f0f0e3162c610f19abae290832 "$profile/source-commit"
jq -e '.source_commit=="fcbdae4b757732f0f0e3162c610f19abae290832" and .source_mode=="retained-release1003-with-lmh-ready-fix" and .parent_copr_build==11240068 and .physical_acceptance==false' "$profile/source-composition.json" >/dev/null
bsdtar -xOf "$archive" boot-payload.json > "$out/emitted-boot-lock.json"
cmp "$boot/lock.json" "$out/emitted-boot-lock.json"
(cd "$boot"; jq -r '.source_assets[]|.sha256+"  "+.path' lock.json | sha256sum -c --strict --quiet)
# Never recompute or replace the emitted source inventories to hide a mismatch.
rm "$stage/inputs/senemos-native-profile-7.3.1.tar.xz" "$stage/inputs/senemos-uke-boot-recipe.tar.xz" "$stage/inputs/senemos-uke-adaptation-7.3.1.tar.xz"
printf '%s\n' 'Fedora source composition: 7.3.1-1006' 'Channel: uke-linux-test:custom:power-20261031 (experimental O82/LMH)' 'Source commit: fcbdae4b757732f0f0e3162c610f19abae290832' 'COPR source build: 11240463' 'Source RPM SHA256: 70f151855f727c9d83c3bedc35ff52dd60cb222b2d741818472dced458bcd60c' 'Source preparation only; native source execution awaits separate approval.' > "$stage/KERNEL-SOURCE-ORIGIN"
