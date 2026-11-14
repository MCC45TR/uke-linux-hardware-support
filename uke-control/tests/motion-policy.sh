#!/usr/bin/bash
# SPDX-License-Identifier: MIT
# Original Fedora policy links in a private unprivileged COPR store only.
set -Eeuo pipefail
export LC_ALL=C
umask 077
ulimit -v 1572864
[[ $# == 4 && ${UKE_POLICY_OFFLINE_FIXTURE:-} == 1 && $EUID != 0 ]]
fedora=$(realpath -e -- "$1")
module=$(realpath -e -- "$2")
helper=$(realpath -e -- "$3")
report=$4
work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT
root=$work/root
mkdir "$root"
mkdir -p "$root/usr/share/selinux/packages"
payload=$root/usr/share/selinux/packages/uke_control_motion.cil
cp -- "$module" "$payload"
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
bash "$helper" --root "$root" --profile targeted install
compiled "$work/installed.cil"
for type in iiosensorproxy_t uke_motion_exec_t uke_motion_runtime_t;do has_type "$work/installed.cil" "$type";done
# Source bytes at priority100 remain exactly the distribution's originals.
hll_hashes > "$work/installed.sha256";cmp "$work/original.sha256" "$work/installed.sha256"
# Reinstall preserves the user's own-module disable, too.
run_semodule -d uke_control_motion
bash "$helper" --root "$root" --profile targeted install
compiled "$work/own-disabled.cil"
! has_type "$work/own-disabled.cil" uke_motion_exec_t
run_semodule --list-modules=full > "$work/own-disabled.list"
awk '$2=="uke_control_motion"&&$NF=="disabled"{found=1}END{exit !found}' "$work/own-disabled.list"
# Explicit fixture user action; the production helper never enables a module.
run_semodule -e uke_control_motion
# Installing the local optional module cannot revive a disabled sensor domain.
run_semodule -d iiosensorproxy
compiled "$work/no-sensor.cil"
! has_type "$work/no-sensor.cil" iiosensorproxy_t
for type in uke_motion_exec_t uke_motion_runtime_t;do has_type "$work/no-sensor.cil" "$type";done
bash "$helper" --root "$root" --profile targeted install
compiled "$work/reinstalled.cil"
! has_type "$work/reinstalled.cil" iiosensorproxy_t
for type in uke_motion_exec_t uke_motion_runtime_t;do has_type "$work/reinstalled.cil" "$type";done
run_semodule --list-modules=full > "$work/reinstalled.list"
for name in bind iiosensorproxy;do awk -v name="$name" '$2==name&&$NF=="disabled"{found=1}END{exit !found}' "$work/reinstalled.list";done
# Foreign priorities and unmarked same-priority modules must not be replaced
# or removed by either production action.
mkdir "$work/foreign"
printf '%s\n' '(type uke_motion_foreign_fixture_t)' > "$work/foreign/uke_control_motion.cil"
store_hashes(){
    (cd "$root/var/lib/selinux/targeted/active";find modules -type f -print0 | sort -z | xargs -0 sha256sum;sha256sum policy.kern)
}
for priority in 500 400;do
    run_semodule -X "$priority" -i "$work/foreign/uke_control_motion.cil"
    store_hashes > "$work/foreign-before.sha256"
    for action in install remove;do
        if bash "$helper" --root "$root" --profile targeted "$action";then echo 'Foreign module was accepted' >&2;exit 1;fi
        store_hashes > "$work/foreign-after.sha256";cmp "$work/foreign-before.sha256" "$work/foreign-after.sha256"
    done
    run_semodule -X "$priority" -r uke_control_motion
    if [[ $priority == 400 ]];then run_semodule -X 400 -i "$payload";fi
 done
# Remove only the new module. Fedora disabled preferences and source survive.
bash "$helper" --root "$root" --profile targeted remove
compiled "$work/removed.cil"
! has_type "$work/removed.cil" uke_motion_exec_t
! has_type "$work/removed.cil" uke_motion_runtime_t
! has_type "$work/removed.cil" iiosensorproxy_t
hll_hashes > "$work/removed.sha256";cmp "$work/original.sha256" "$work/removed.sha256"
run_semodule --list-modules=full > "$work/removed.list"
for name in bind iiosensorproxy;do awk -v name="$name" '$2==name&&$NF=="disabled"{found=1}END{exit !found}' "$work/removed.list";done
printf '%s\n' 'Original Fedora policy bytes preserved; optional module links with sensor domain enabled and disabled; disabled original and owned preferences survive reinstall; foreign ownership is refused without store changes; owned removal is confined.' > "$report"
