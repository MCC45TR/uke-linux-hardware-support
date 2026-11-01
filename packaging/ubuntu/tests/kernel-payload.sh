#!/bin/bash
# Synthetic byte/DT fixtures only. No candidate binary, kernel or module runs.
set -euo pipefail
root=$(git -C "$(dirname "$0")" rev-parse --show-toplevel)
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
release=7.3.1-1006u1
tree=$work/root/usr/lib/uke-kernel-candidates/$release
mkdir -p "$tree/modules/kernel"
truncate -s 2048 "$tree/Image"
printf '\101\122\115\144' | dd of="$tree/Image" bs=1 seek=56 conv=notrunc status=none
printf '\000\010\000\000\000\000\000\000' | dd of="$tree/Image" bs=1 seek=16 conv=notrunc status=none
printf 'Linux version %s fixture\000' "$release" | dd of="$tree/Image" bs=1 seek=64 conv=notrunc status=none
printf '%s\n' "$release" > "$tree/kernel-release"
printf '%s\n' 'Fedora source composition: 7.3.1-1006' > "$tree/KERNEL-SOURCE-ORIGIN"
printf '%s\n' candidate_only=true bootable=false stable_admitted=false > "$tree/admission"
printf '%s\n' CONFIG_ARM64=y CONFIG_QCOM_UKE_ABL_DT=y CONFIG_INITRAMFS_FORCE=y CONFIG_MODULE_SIG=y CONFIG_MODULE_SIG_FORCE=y 'CONFIG_INITRAMFS_SOURCE=""' > "$tree/config"
# Reuse a public system CA certificate only as an X.509 parsing fixture.
# No key is created or read, and this certificate cannot qualify a module.
certificate=$(find /etc/ssl/certs -type f -name '*.pem' -print -quit)
[[ -n $certificate ]] || certificate=/etc/ssl/certs/ca-certificates.crt
openssl x509 -in "$certificate" -outform DER -out "$tree/module-signing.x509"
for file in modules.dep modules.builtin modules.order; do printf 'fixture\n' > "$tree/modules/$file"; done
dt() { printf '/dts-v1/; / { compatible = "%s", "qcom,sm7675"; };\n' "$1" | dtc -I dts -O dtb -o "$tree/sm7675-xiaomi-uke.dtb"; }
refresh() {
 (cd "$tree"; sha256sum Image config kernel-release KERNEL-SOURCE-ORIGIN admission module-signing.x509 > core.sha256; sha256sum sm7675-xiaomi-uke.dtb > dtbs.sha256; find modules -type f -print0 | sort -z | xargs -0 sha256sum > modules.sha256)
}
reject() {
 if bash "$root/packaging/ubuntu/inspect-kernel-candidate.sh" "$work/root" "$release" > "$work/log" 2>&1; then echo 'Invalid candidate accepted' >&2; exit 1; fi
}
dt xiaomi,uke
refresh
reject
grep -q 'Empty module candidate refused' "$work/log"
dt xiaomi,nabu
refresh
reject
grep -q 'Foreign DT compatible' "$work/log"
dt xiaomi,uke
printf X | dd of="$tree/Image" bs=1 seek=56 conv=notrunc status=none
refresh
reject
grep -q 'Invalid ARM64 Image magic' "$work/log"
printf '\101' | dd of="$tree/Image" bs=1 seek=56 conv=notrunc status=none
refresh
printf corruption >> "$tree/Image"
reject
grep -q 'computed checksum did NOT match' "$work/log"
refresh
ln -s /etc/passwd "$tree/foreign-link"
reject
rm "$tree/foreign-link"
printf '%s\n' '0000000000000000000000000000000000000000000000000000000000000000  ../../outside' > "$tree/modules.sha256"
reject
for package in uke-firmware uke-kernel-boot uke-oem-resources; do
 status=0
 bash "$root/packaging/ubuntu/prepare-source.sh" "$package" "$work/$package" > "$work/gate" 2>&1 || status=$?
 [[ $status == 78 && ! -e $work/$package ]]
 grep -q '^Unsupported:' "$work/gate"
done
printf '%s\n' 'PASS: kernel payload refusal fixtures and publication gates; no real kernel/module or boot admission claimed'
