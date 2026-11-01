#!/bin/bash
# Assemble inert native DEBs from an independently checked server cohort.
# This is artifact packaging, not a new source build or root-image admission.
set -euo pipefail
export LC_ALL=C
[[ $# == 5 ]] || { echo 'Usage: package-boot-trial.sh COHORT BUILD_EVIDENCE EARLY_MODULE_LIST multiboot|singleboot NEW_OUTPUT' >&2; exit 2; }
cohort=$(realpath "$1")
evidence=$(realpath "$2")
early=$(realpath "$3")
layout=$4
output=$(realpath -m "$5")
[[ $layout == multiboot || $layout == singleboot ]]
[[ ! -e $output ]]
here=$(cd -- "$(dirname -- "$0")"; pwd)
copyright=$here/recipes/uke-linux-kernel/debian/copyright
[[ -f $copyright ]] || copyright=$here/copyright
[[ -f $copyright && ! -L $copyright ]]
# Verify certificate identity, every module, both boot readbacks and embedded
# payload provenance before creating any package staging tree.
bash "$here/inspect-boot-trial.sh" "$cohort" "$evidence" "$early"
work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT
release=7.3.1-1006u1
version=7.3.1-1006ubuntu1~tenacious1
destination=usr/lib/uke-kernel-trials/$release
config=$cohort/config
[[ -f $config ]] || config=$cohort/.config
tar -xJf "$cohort/modules.tar.xz" -C "$work" --no-same-owner --no-same-permissions
mkdir "$output"
: > "$work/packages.jsonl"
for role in kernel dtb modules boot; do
 case $role in
  kernel) package=uke-kernel-trial-core;;
  dtb) package=uke-kernel-trial-dtbs;;
  modules) package=uke-kernel-trial-modules;;
  boot) package=uke-kernel-trial-boot;;
 esac
 tree=$work/$package
 install -d "$tree/DEBIAN" "$tree/$destination" "$tree/usr/share/doc/$package"
 {
  printf 'Package: %s\nVersion: %s\nArchitecture: arm64\nSection: kernel\nPriority: optional\n' "$package" "$version"
  printf 'Maintainer: MCC45TR <75160848+MCC45TR@users.noreply.github.com>\n'
  if [[ $role == boot ]]; then
   printf 'Depends: uke-kernel-trial-core (= %s), uke-kernel-trial-dtbs (= %s), uke-kernel-trial-modules (= %s)\n' "$version" "$version" "$version"
  fi
  printf 'Description: Inert Uke Ubuntu kernel trial %s payload\n' "$role"
  printf ' Exact shared release1006 source; staged outside live boot/module paths.\n No activation hook, qualified hardware ABI, stable or root-image admission.\n'
 } > "$tree/DEBIAN/control"
 case $role in
  kernel)
   install -m644 "$cohort/Image" "$cohort/module-signing.x509" "$tree/$destination/"
   install -m644 "$config" "$tree/$destination/config"
   printf '%s\n' "$release" > "$tree/$destination/kernel-release"
   printf '%s\n' 'fcbdae4b757732f0f0e3162c610f19abae290832' > "$tree/$destination/source-commit";;
  dtb) install -m644 "$cohort/sm7675-xiaomi-uke.dtb" "$tree/$destination/";;
  modules) cp -a "$work/lib/modules/$release" "$tree/$destination/modules";;
  boot)
   install -d "$tree/$destination/boot" "$tree/$destination/provenance"
   install -m644 "$cohort/linux.img" "$cohort/userdata.img" "$cohort/boot-manifest" "$cohort/boot-header.json" "$cohort/modules.sha256" "$tree/$destination/boot/"
   for file in payload-provenance.tsv build-debs.tsv base-links.tsv; do
    install -m644 "$evidence/$file" "$tree/$destination/provenance/"
   done
   printf '%s\n' candidate_only=true embedded_initramfs=true automatic_boot_selection=false root_image_admitted=false stable_admitted=false > "$tree/$destination/admission";;
 esac
 # Keep source licensing metadata with every role. No builder path or private
 # log is copied; the existing public packaging copyright names source rights.
 install -m644 "$copyright" "$tree/usr/share/doc/$package/copyright"
 if [[ -d $cohort/licenses && ! -L $cohort/licenses ]]; then
  [[ -z $(find "$cohort/licenses" ! -type d ! -type f -print -quit) ]]
  cp -a "$cohort/licenses" "$tree/usr/share/doc/$package/licenses"
 else
  printf '%s\n' 'Private inspection only. Complete embedded-userspace copyright and license collection is absent from the original trial artifact. Redistribution remains blocked.' > "$tree/usr/share/doc/$package/REDISTRIBUTION-BLOCKED"
 fi
 (cd "$tree/$destination"; find . -type f -print0 | sort -z | xargs -0 sha256sum > "$work/role.sha256")
 install -m644 "$work/role.sha256" "$tree/usr/share/doc/$package/payload.sha256"
 file=${package}_${version}_arm64.deb
 dpkg-deb --root-owner-group --uniform-compression -Zxz -z6 --build "$tree" "$output/$file"
 jq -nc --arg role "$role" --arg name "$package" --arg file "$file" --arg sha256 "$(sha256sum "$output/$file" | cut -d ' ' -f1)" '{role:$role,name:$name,file:$file,sha256:$sha256}' >> "$work/packages.jsonl"
done
jq -s --arg version "$version" --arg release "$release" --arg layout "$layout" '{schema:"uke-ubuntu-boot-cohort-v1",series:"tenacious",architecture:"arm64",compatible:"xiaomi,uke",layout:$layout,kernel_release:$release,version:$version,packages:.,initramfs:{policy:"review-required"},automatic_boot_selection:false,stable_admitted:false}' "$work/packages.jsonl" > "$output/cohort.json"
bash "$here/validate-cohort.sh" "$output/cohort.json" "$output"
# Independently extract each completed DEB and compare its complete file set
# to the staging inventory. Never install or execute a maintainer script.
mkdir "$work/readback"
for deb in "$output/"*.deb; do dpkg-deb -x "$deb" "$work/readback"; done
for role_file in "$work/"uke-kernel-trial-*/usr/share/doc/*/payload.sha256; do
 (cd "$work/readback/$destination"; sha256sum --check --strict "$role_file")
done
[[ ! -e $work/readback/boot && ! -e $work/readback/etc && ! -e $work/readback/usr/lib/modules ]]
[[ -z $(find "$work/readback" \( -iname '*python*' -o -name '*.py' -o -name '*.pyc' -o -name '*.pyo' \) -print -quit) ]]
for package_tree in "$work/"uke-kernel-trial-*; do
 [[ $(find "$package_tree/DEBIAN" -type f -printf '%f\n') == control ]]
done
printf '%s\n' artifact_packaging=true source_build=false root_image_admitted=false redistribution_admitted=false publication=false tablet_actions=false > "$output/qualification"
(cd "$output"; sha256sum ./*.deb cohort.json qualification > SHA256SUMS)
echo 'Four inert trial packages passed byte readback and exact dependencies; no image or hardware admission.'
