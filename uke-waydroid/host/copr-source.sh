#!/usr/bin/env bash
# COPR source worker only; no tablet policy preparation or native compilation.
set -Eeuo pipefail
export LC_ALL=C
test ! -e /etc/senemos/kernel-update.conf
owner=$(cd -- "$(dirname -- "$0")/../.." && pwd)
output=$(realpath -m "${1:?COPR result directory}")
mkdir -p "$output"
# The mock source worker owns its isolated Fedora SELinux store. Compile/link
# against all original modules without reloading the worker/host kernel policy.
semodule -n -X 400 -i "$owner/uke-waydroid/selinux/uke_waydroid_binder.cil"
semodule -lfull | awk '$2=="uke_waydroid_binder" {found=1;print} END {exit !found}'
printf 'Complete Fedora policy links the Waydroid Binder genfscon module\n'
git -C "$owner" archive --format=tar --prefix=uke-waydroid-integration-0.1.0/ \
    HEAD uke-waydroid | xz -T1 > "$output/uke-waydroid-integration-0.1.0.tar.xz"
install -m644 "$owner/uke-waydroid/packaging/uke-waydroid-integration.spec" \
    "$output/uke-waydroid-integration.spec"
printf 'Source commit: %s\n' "$(git -C "$owner" rev-parse HEAD)"
sha256sum "$output/uke-waydroid-integration-0.1.0.tar.xz" \
    "$owner/uke-waydroid/selinux/uke_waydroid_binder.cil"
