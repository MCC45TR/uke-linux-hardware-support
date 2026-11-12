#!/usr/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# Real libsemanage operations in private, unprivileged COPR fixture roots only.
set -Eeuo pipefail
export LC_ALL=C
umask 077
ulimit -v 1572864
[[ $# == 7 && ${UKE_POLICY_OFFLINE_FIXTURE:-} == 1 && $EUID != 0 ]]
fedora=$1
fork=$2
composer=$3
canonical=$4
rgb=$5
ssc=$6
result=$7
mkdir -p "$result"
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
mkdir "$work/fedora" "$work/fork"
extract() {
    rpm2cpio "$1" | (cd "$2"; cpio -idm --quiet --no-absolute-filenames --no-preserve-owner \
        './etc/selinux/*' './var/lib/selinux/*')
    [[ -z $(find "$2" -type l -print -quit) ]]
}
extract "$fedora" "$work/fedora"
extract "$fork" "$work/fork"
for root in "$work/fedora" "$work/fork"; do
    printf '%s\n' 'module-store = direct' 'policy-version = 35' 'expand-check = 1' \
        > "$root/etc/selinux/semanage.conf"
done
run_semodule() {
    local root=$1
    shift
    [[ $root == "$work/fedora" || $root == "$work/fork" ]]
    timeout -k 15s 300s semodule -n -p "$root" -s targeted "$@"
}
hll_hashes() {
    (cd "$1/var/lib/selinux/targeted/active/modules/100";
        find . -name hll -type f -print0 | sort -z | xargs -0 sha256sum)
}
disabled() {
    run_semodule "$1" --list-modules=full | awk '$2=="bind"{print}'
}
compiled() {
    local root=$1 out=$2
    checkpolicy -b -M -C -o "$out" "$root/var/lib/selinux/targeted/active/policy.kern"
}
normalize() {
    timeout -k 15s 300s "$canonical" "$1" < "$2" | sort -u > "$3"
}
equal() {
    if ! cmp "$1" "$2"; then
        diff -u "$1" "$2" | awk 'NR<=80{print}' || true
        return 1
    fi
}
compose() {
    local root=$1 body=$2
    bzip2 -dc "$root/var/lib/selinux/targeted/active/modules/100/base/cil" > "$work/base.cil"
    gawk -f "$composer" "$work/base.cil" "$body" > "$work/uke_kernel_compat.cil"
}
apply() {
    local root=$1
    if grep -q '^[[:space:]]*(' "$work/uke_kernel_compat.cil"; then
        run_semodule "$root" -X 400 -i "$work/uke_kernel_compat.cil"
    else
        run_semodule "$root" --list-modules=full > "$work/modules.list"
        if awk '$2=="uke_kernel_compat"{found=1}END{exit !found}' "$work/modules.list"; then
            run_semodule "$root" -X 400 -r uke_kernel_compat
        fi
    fi
}

printf '%s\n' 'Initialize original Fedora and historical-fork module stores with exact RGB/SSC policy inputs.'
for root in "$work/fedora" "$work/fork"; do
    hll_hashes "$root" > "$root.hll.before"
    run_semodule "$root" -X 400 -i "$rgb" -i "$ssc"
done

# Capture a domain that will disappear, then disable its owning module. The
# staged composer must tolerate this stale snapshot without reviving a type.
compiled "$work/fedora" "$work/stale.cil"
for root in "$work/fedora" "$work/fork"; do
    run_semodule "$root" -d bind
    disabled "$root" > "$root.disabled"
    grep -q disabled "$root.disabled"
done
compiled "$work/fedora" "$work/baseline.cil"
normalize old "$work/baseline.cil" "$work/baseline.old"
compiled "$work/fork" "$work/fork-reference.cil"
normalize full "$work/fork-reference.cil" "$work/fork-reference.full"

printf '%s\n' 'Install compatibility with a stale domain snapshot; preserve disabled modules and all existing records.'
compose "$work/fedora" "$work/stale.cil"
apply "$work/fedora"
compiled "$work/fedora" "$work/composed.cil"
normalize old "$work/composed.cil" "$work/composed.old"
equal "$work/baseline.old" "$work/composed.old"
normalize full "$work/composed.cil" "$work/composed.full"
compose "$work/fedora" "$work/composed.cil"
apply "$work/fedora"
compiled "$work/fedora" "$work/refreshed.cil"
normalize full "$work/refreshed.cil" "$work/refreshed.full"
equal "$work/composed.full" "$work/refreshed.full"
hll_hashes "$work/fedora" > "$work/fedora.hll.after"
equal "$work/fedora.hll.before" "$work/fedora.hll.after"
disabled "$work/fedora" > "$work/disabled.after"
equal "$work/fedora.disabled" "$work/disabled.after"

printf '%s\n' 'Reinstall the same policy module using original base definitions and an already-composed linked policy.'
cp "$work/uke_kernel_compat.cil" "$work/reinstall-before.cil"
compose "$work/fedora" "$work/refreshed.cil"
equal "$work/reinstall-before.cil" "$work/uke_kernel_compat.cil"
apply "$work/fedora"
compiled "$work/fedora" "$work/reinstalled.cil"
normalize full "$work/reinstalled.cil" "$work/reinstalled.full"
equal "$work/refreshed.full" "$work/reinstalled.full"

# These two pinned payloads must have the same original module-name set for
# this payload-overlay fixture to model RPM replacement without deleting state.
for root in "$work/fedora" "$work/fork"; do
    (cd "$root/var/lib/selinux/targeted/active/modules/100";
        find . -name hll -type f | sort) > "$root.module-names"
done
equal "$work/fedora.module-names" "$work/fork.module-names"

printf '%s\n' 'Restore the supported fork cohort first, then remove the redundant compatibility module.'
cp "$work/fedora/var/lib/selinux/targeted/active/policy.kern" "$work/before-fork.35"
checkpolicy -b -M -C -o "$work/before-fork.cil" "$work/before-fork.35"
cp -a "$work/fork/var/lib/selinux/targeted/active/modules/100/." \
    "$work/fedora/var/lib/selinux/targeted/active/modules/100/"
compose "$work/fedora" "$work/before-fork.cil"
! grep -q '^[[:space:]]*(' "$work/uke_kernel_compat.cil"
apply "$work/fedora"
compiled "$work/fedora" "$work/rollback.cil"
normalize full "$work/rollback.cil" "$work/rollback.full"
equal "$work/fork-reference.full" "$work/rollback.full"
disabled "$work/fedora" > "$work/disabled.rollback"
equal "$work/fedora.disabled" "$work/disabled.rollback"

printf '%s\n' 'Reupgrade original Fedora and reinstall compatibility, retaining the disabled-module preference.'
mkdir "$work/fresh"
extract "$fedora" "$work/fresh"
cp -a "$work/fresh/var/lib/selinux/targeted/active/modules/100/." \
    "$work/fedora/var/lib/selinux/targeted/active/modules/100/"
compose "$work/fedora" "$work/rollback.cil"
apply "$work/fedora"
compiled "$work/fedora" "$work/reupgrade.cil"
normalize full "$work/reupgrade.cil" "$work/reupgrade.full"
equal "$work/reinstalled.full" "$work/reupgrade.full"
disabled "$work/fedora" > "$work/disabled.reupgrade"
equal "$work/fedora.disabled" "$work/disabled.reupgrade"
hll_hashes "$work/fedora" > "$work/fedora.hll.final"
equal "$work/fedora.hll.before" "$work/fedora.hll.final"

sha256sum "$work/composed.full" "$work/refreshed.full" "$work/reinstalled.full" \
    "$work/rollback.full" "$work/fork-reference.full" "$work/reupgrade.full" \
    | sed "s@$work/@@g" > "$result/store-representation.sha256"
printf '%s\n' '{"offline_only":true,"targeted_store":true,"current_rgb_ssc_inputs":true,"disabled_bind_preserved":true,"stale_type_snapshot":true,"old_permission_records_equal":true,"reinstall_records_equal":true,"fork_first_rollback_equal":true,"original_reupgrade_equal":true,"original_hll_sources_unchanged":true,"device_policy_loaded":false,"actual_dnf_transaction":false}' > "$result/store-result.json"
printf '%s\n' 'Private native module-store lifecycle passed; no host policy loaded. Actual RPM/DNF ordering remains a separate gate.'
