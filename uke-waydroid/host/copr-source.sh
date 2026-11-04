#!/usr/bin/env bash
# COPR source worker only; no tablet policy preparation or native compilation.
set -Eeuo pipefail
export LC_ALL=C
test ! -e /etc/senemos/kernel-update.conf
owner=$(cd -- "$(dirname -- "$0")/../.." && pwd)
output=$(realpath -m "${1:?COPR result directory}")
mkdir -p "$output"
# Custom source workers are unprivileged. Extract the exact installed Fedora
# policy RPM into a private root instead of accessing the protected mock store.
stage=$(mktemp -d)
trap 'find "$stage" -depth -delete' EXIT
mkdir -p "$stage/rpms" "$stage/root"
policy_nvr=$(rpm -q --qf '%{NAME}-%{VERSION}-%{RELEASE}.%{ARCH}' selinux-policy-targeted)
dnf -y --setopt="cachedir=$stage/cache" download --destdir "$stage/rpms" "$policy_nvr"
mapfile -t policy_rpms < <(find "$stage/rpms" -maxdepth 1 -type f -name '*.rpm')
[[ ${#policy_rpms[@]} == 1 ]]
[[ $(rpm -qp --qf '%{NAME}-%{VERSION}-%{RELEASE}.%{ARCH}' "${policy_rpms[0]}") == "$policy_nvr" ]]
rpmkeys --checksig --nosignature --verbose "${policy_rpms[0]}"
(cd "$stage/root"; rpm2cpio "${policy_rpms[0]}" | cpio -idm --quiet --no-absolute-filenames)
install -Dm644 /etc/selinux/config "$stage/root/etc/selinux/config"
# Link against all original modules without loading worker or host policy.
semodule -p "$stage/root" -S /var/lib/selinux -s targeted -n -X 400 \
    -X 200 -i /usr/share/selinux/targeted/waydroid.pp \
    -X 400 -i "$owner/uke-waydroid/selinux/uke_waydroid_binder.cil"
semodule -p "$stage/root" -S /var/lib/selinux -s targeted -lfull | \
    awk '$2=="uke_waydroid_binder" {found=1;print} END {exit !found}'
printf 'Complete Fedora policy links the Waydroid Binder genfscon module\n'
git -C "$owner" archive --format=tar --prefix=uke-waydroid-integration-0.1.0/ \
    HEAD uke-waydroid | xz -T1 > "$output/uke-waydroid-integration-0.1.0.tar.xz"
install -m644 "$owner/uke-waydroid/packaging/uke-waydroid-integration.spec" \
    "$output/uke-waydroid-integration.spec"
printf 'Source commit: %s\n' "$(git -C "$owner" rev-parse HEAD)"
sha256sum "$output/uke-waydroid-integration-0.1.0.tar.xz" \
    "$owner/uke-waydroid/selinux/uke_waydroid_binder.cil"
