#!/bin/bash
# Verify boot package identity/coherency without installing or selecting a boot.
set -euo pipefail
manifest=${1:?Usage: validate-cohort.sh MANIFEST ARTIFACT_DIRECTORY}
artifacts=${2:?Missing artifact directory}
jq -e '
 .schema == "uke-ubuntu-boot-cohort-v1" and .series == "tenacious" and
 .architecture == "arm64" and .compatible == "xiaomi,uke" and
 (.layout == "multiboot" or .layout == "singleboot") and
 (.kernel_release | test("^[A-Za-z0-9.+_-]+$")) and
 (.version | test("^[0-9][A-Za-z0-9.+:~_-]+$")) and
 (.packages | length == 4) and
 ([.packages[].role] | sort == ["boot", "dtb", "kernel", "modules"]) and
 ([.packages[].name] | unique | length == 4) and
 all(.packages[]; (.name|test("^[a-z0-9][a-z0-9+.-]+$")) and
   (.file|test("^[a-z0-9][A-Za-z0-9+._~:-]*\\.deb$")) and
   (.sha256|test("^[a-f0-9]{64}$"))) and
 .initramfs.policy == "review-required" and
 .automatic_boot_selection == false and .stable_admitted == false
' "$manifest" >/dev/null
version=$(jq -r .version "$manifest")
while IFS=$'\t' read -r name file hash; do
 [[ -f $artifacts/$file && ! -L $artifacts/$file ]] || exit 1
 printf '%s  %s\n' "$hash" "$artifacts/$file" | sha256sum -c -
 [[ $(dpkg-deb -f "$artifacts/$file" Package) == "$name" ]]
 [[ $(dpkg-deb -f "$artifacts/$file" Version) == "$version" ]]
 [[ $(dpkg-deb -f "$artifacts/$file" Architecture) == arm64 ]]
done < <(jq -r '.packages[] | [.name,.file,.sha256] | @tsv' "$manifest")
boot=$(jq -r '.packages[]|select(.role=="boot")|.file' "$manifest")
depends=$(dpkg-deb -f "$artifacts/$boot" Depends)
while read -r name; do
 # Reject alternatives or loose >= dependencies in the boot dependency closure.
 printf '%s\n' "$depends" | tr ',' '\n' | sed 's/^ *//;s/ *$//' | grep -Fx "$name (= $version)" >/dev/null
done < <(jq -r '.packages[]|select(.role!="boot")|.name' "$manifest")
printf '%s\n' 'PASS: cohort artifact identities and exact boot dependencies; initramfs, boot and hardware admission remain pending'
