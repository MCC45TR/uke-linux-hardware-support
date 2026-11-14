#!/usr/bin/bash
# SPDX-License-Identifier: MIT
# Original Fedora policy links in a private unprivileged COPR store only.
set -Eeuo pipefail
export LC_ALL=C
umask 077
ulimit -v 1572864
[[ $# == 3 && ${UKE_POLICY_OFFLINE_FIXTURE:-} == 1 && $EUID != 0 ]]
fedora=$(realpath -e -- "$1")
module=$(realpath -e -- "$2")
report=$3
work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT
root=$work/root
mkdir "$root"
rpm2cpio "$fedora" | (cd "$root";cpio -idm --quiet --no-absolute-filenames --no-preserve-owner './etc/selinux/*' './var/lib/selinux/*')
[[ -z $(find "$root" -type l -print -quit) ]]
printf '%s\n' 'module-store = direct' 'policy-version = 35' 'expand-check = 1' > "$root/etc/selinux/semanage.conf"
run_semodule() { timeout -k 15s 300s semodule -n -p "$root" -s targeted "$@"; }
hll_hashes() {
    (cd "$root/var/lib/selinux/targeted/active/modules/100";find . -name hll -type f -print0 | sort -z | xargs -0 sha256sum)
}
compiled() { checkpolicy -b -M -C -o "$1" "$root/var/lib/selinux/targeted/active/policy.kern"; }
has_type() { grep -Eq "^[[:space:]]*\(type $2\)" "$1"; }
hll_hashes > "$work/original.sha256"
run_semodule --list-modules=full > "$work/original.list"
! awk '$2=="uke_control_motion"{found=1}END{exit !found}' "$work/original.list"
# An unrelated user-disabled module must remain disabled throughout.
run_semodule -d bind
run_semodule --list-modules=full > "$work/disabled.list"
awk '$2=="bind"&&$NF=="disabled"{found=1}END{exit !found}' "$work/disabled.list"
run_semodule -X 400 -i "$module"
compiled "$work/installed.cil"
for type in iiosensorproxy_t uke_motion_exec_t uke_motion_runtime_t;do has_type "$work/installed.cil" "$type";done
# Source bytes at priority100 remain exactly the distribution's originals.
hll_hashes > "$work/installed.sha256";cmp "$work/original.sha256" "$work/installed.sha256"
# Installing the local optional module cannot revive a disabled sensor domain.
run_semodule -d iiosensorproxy
compiled "$work/no-sensor.cil"
! has_type "$work/no-sensor.cil" iiosensorproxy_t
for type in uke_motion_exec_t uke_motion_runtime_t;do has_type "$work/no-sensor.cil" "$type";done
run_semodule -X 400 -i "$module"
compiled "$work/reinstalled.cil"
! has_type "$work/reinstalled.cil" iiosensorproxy_t
for type in uke_motion_exec_t uke_motion_runtime_t;do has_type "$work/reinstalled.cil" "$type";done
run_semodule --list-modules=full > "$work/reinstalled.list"
for name in bind iiosensorproxy;do awk -v name="$name" '$2==name&&$NF=="disabled"{found=1}END{exit !found}' "$work/reinstalled.list";done
# Remove only the new module. Fedora disabled preferences and source survive.
run_semodule -X 400 -r uke_control_motion
compiled "$work/removed.cil"
! has_type "$work/removed.cil" uke_motion_exec_t
! has_type "$work/removed.cil" uke_motion_runtime_t
! has_type "$work/removed.cil" iiosensorproxy_t
hll_hashes > "$work/removed.sha256";cmp "$work/original.sha256" "$work/removed.sha256"
run_semodule --list-modules=full > "$work/removed.list"
for name in bind iiosensorproxy;do awk -v name="$name" '$2==name&&$NF=="disabled"{found=1}END{exit !found}' "$work/removed.list";done
printf '%s\n' 'Original Fedora policy bytes preserved; optional module links with sensor domain enabled and disabled; disabled distribution preferences survive reinstall/removal.' > "$report"
