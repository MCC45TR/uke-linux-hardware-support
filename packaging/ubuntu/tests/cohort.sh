#!/bin/bash
set -euo pipefail
umask 022
root=$(git -C "$(dirname "$0")" rev-parse --show-toplevel)
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
packages='[]'
for role in kernel dtb modules boot; do
 name=uke-fixture-$role
 mkdir -p "$work/$name/DEBIAN"
 { printf 'Package: %s\nVersion: 1.0-1\nArchitecture: arm64\nMaintainer: Uke <75160848+MCC45TR@users.noreply.github.com>\nDescription: Static cohort metadata fixture\n' "$name"
   if [[ $role == boot ]]; then printf 'Depends: uke-fixture-kernel (= 1.0-1), uke-fixture-dtb (= 1.0-1), uke-fixture-modules (= 1.0-1)\n'; fi
 } > "$work/$name/DEBIAN/control"
 dpkg-deb --build --root-owner-group "$work/$name" "$work/$name.deb" >/dev/null
 hash=$(sha256sum "$work/$name.deb" | cut -d ' ' -f1)
 packages=$(jq --arg name "$name" --arg role "$role" --arg hash "$hash" '. + [{name:$name,role:$role,file:($name+".deb"),sha256:$hash}]' <<< "$packages")
done
jq -n --argjson packages "$packages" '{schema:"uke-ubuntu-boot-cohort-v1",series:"tenacious",architecture:"arm64",compatible:"xiaomi,uke",layout:"multiboot",kernel_release:"fixture-only",version:"1.0-1",packages:$packages,initramfs:{policy:"review-required"},automatic_boot_selection:false,stable_admitted:false}' > "$work/cohort.json"
bash "$root/packaging/ubuntu/validate-cohort.sh" "$work/cohort.json" "$work"
for mutation in '.series="noble"' '.architecture="amd64"' '.compatible="xiaomi,nabu"' '.stable_admitted=true' '.automatic_boot_selection=true' '.version="2.0-1"' '.packages[0].file="../foreign.deb"'; do
 jq "$mutation" "$work/cohort.json" > "$work/rejected.json"
 if bash "$root/packaging/ubuntu/validate-cohort.sh" "$work/rejected.json" "$work" >/dev/null 2>&1; then exit 1; fi
done
printf corrupt >> "$work/uke-fixture-dtb.deb"
if bash "$root/packaging/ubuntu/validate-cohort.sh" "$work/cohort.json" "$work" >/dev/null 2>&1; then exit 1; fi
printf '%s\n' 'PASS: foreign series/architecture/device, promotion, auto-selection, mismatched version, traversal and corrupted DT package refused'
