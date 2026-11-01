#!/usr/bin/env bash
# COPR source worker only; construct a temporary archive without installing RPMs or booting it.
set -Eeuo pipefail
[[ ${UKE_COPR_GNU_RETURN:-} == 1 ]]
[[ $(rpm --eval '%{_host_cpu}') == aarch64 && $(rpm --eval '%{fedora}') == 46 ]]
test ! -e /etc/senemos/kernel-update.conf
package=${1:?signed candidate recovery RPM}
[[ $(rpm -qp --qf '%{NAME}' "$package") == uke-boot-recovery ]]
stage=$(mktemp -d)
trap 'rm -rf -- "$stage"' EXIT
mkdir -p "$stage/payload" "$stage/unpacked"
rpm2cpio "$package" > "$stage/payload.cpio"
cpio -t --quiet < "$stage/payload.cpio" > "$stage/members"
awk '/^\/|(^|\/)\.\.($|\/)/{bad=1}END{exit bad}' "$stage/members"
(cd "$stage/payload";cpio -idm --quiet --no-absolute-filenames < "$stage/payload.cpio")
guard=$stage/payload/usr/libexec/senemos-uke/uke-boot-guard
[[ -x $guard ]]
grep -Fq 'inst_multiple /usr/libexec/senemos-uke/uke-boot-guard' "$stage/payload/usr/lib/dracut/modules.d/93uke-boot-guard/module-setup.sh"
# The installed Fedora runtime supplies libraries; optional GDB files are not requested.
[[ $(rpm -qf --qf '%{NAME}' /usr/lib64/libstdc++.so.6) == libstdc++ ]]
# Exercise dracut's native dependency copying; the full tablet image remains a separate gate.
dracut --force --no-kernel --no-hostonly --modules 'base systemd' --install "$guard" --include "$guard" /usr/libexec/senemos-uke/uke-boot-guard "$stage/initrd.img"
(cd "$stage/unpacked";lsinitrd --unpack "$stage/initrd.img")
find "$stage/unpacked" -printf '%P\n' | LC_ALL=C sort > "$stage/paths"
grep -Fx 'usr/libexec/senemos-uke/uke-boot-guard' "$stage/paths" >/dev/null
grep -Eq '(^|/)libstdc[+][+][.]so[.]6' "$stage/paths"
if grep -Ei '(^|/)(python[0-9.]*|pypy[^/]*|libpython[^/]*|__pycache__)(/|$)|[.]py([co])?$' "$stage/paths";then
    echo 'Python payload entered the native boot-guard initramfs' >&2;exit 1
fi
echo 'UKE_GNU_INITRAMFS original_fedora_libstdcxx=true native_guard_dependency_fixture=true python_files=0 kernel_included=false full_tablet_image=false booted=false'
