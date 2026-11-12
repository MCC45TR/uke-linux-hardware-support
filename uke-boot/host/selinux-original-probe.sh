#!/usr/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# Offline COPR policy-data qualification. Never load a policy on the host.
set -Eeuo pipefail
export LC_ALL=C
# Keep policy experiments bounded on the default COPR worker. Resource refusal
# is a failed qualification, never a reason to alter or load the device policy.
ulimit -v 1572864
[[ $# == 5 ]]
baseline_srpm=$1
fork_rpm=$2
fedora_rpm=$3
composer=$4
result=$5
mkdir -p "$result"
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
cd "$work"
rpm2cpio "$baseline_srpm" | cpio --quiet -i --to-stdout ./fedora-targeted-baseline.35 > baseline.35
rpm2cpio "$fork_rpm" | cpio --quiet -i --to-stdout ./etc/selinux/targeted/policy/policy.35 > fork.35
rpm2cpio "$fedora_rpm" | cpio --quiet -i --to-stdout ./etc/selinux/targeted/policy/policy.35 > fedora.35
echo '63ce943b9b060e81b714501cce086bc509f0b9f6457e2fb468824aa35482538c  baseline.35' | sha256sum -c -
for item in baseline fork fedora; do
    checkpolicy -b -M -C -o "$item.cil" "$item.35"
done
[[ $(grep -Ec '^[[:space:]]*\(allow .* \(io_uring \([^)]*allowed' fork.cil) == 100 ]]
[[ $(grep -Ec '^[[:space:]]*\(allow .* \(system \([^)]*firmware_load' fork.cil) == 8 ]]
for item in baseline fedora; do
    gawk -f "$composer" "$item.cil" > "$item-compat.cil"
    printf 'Compile offline composition: %s\n' "$item"
    timeout -k 15s 300s secilc -M true -c 35 -f /dev/null -o "$item-compat.35" "$item.cil" "$item-compat.cil"
done
gawk -f "$composer" fork.cil > already-adopted.cil
! grep -q '^[[:space:]]*(' already-adopted.cil

# Compare decisions by names, including Boolean conditions and transitions;
# the common layout and permission numbers are deliberately different.
differences=(--allow --auditallow --dontaudit --allowxperm --auditallowxperm
    --dontauditxperm --type_trans --type_change --type_member --role_allow
    --role_trans --range_trans --constrain --mlsconstrain --validatetrans
    --mlsvalidatetrans --initialsid --fs_use --genfscon --netifcon --nodecon
    --portcon --default --property --polcap --typebounds)
printf '%s\n' 'Compare complete historical decisions by name and condition.'
timeout -k 15s 300s sediff --stats "${differences[@]}" fork.35 baseline-compat.35 > "$result/fork-parity.txt"
cat "$result/fork-parity.txt"
# Explicitly selected sediff categories print zero-count headers too.
# Require recognized, exclusively zero-count headers; reject unknown output.
awk '
    NF {
        seen++
        if ($0 !~ /^[[:alnum:] _]+ \(0 (Added|Removed|Modified)(, 0 (Added|Removed|Modified))*\)$/)
            bad=1
    }
    END { exit bad || !seen }
' "$result/fork-parity.txt"

for item in baseline-compat fedora-compat; do
    seinfo --polcap -x "$item.35" | sort > "$item.cap"
    seinfo --permissive -x "$item.35" | sort > "$item.permissive"
done
for item in baseline fedora; do
    seinfo --polcap -x "$item.35" | sort > "$item.cap"
    seinfo --permissive -x "$item.35" | sort > "$item.permissive"
    cmp "$item.cap" "$item-compat.cap"
    cmp "$item.permissive" "$item-compat.permissive"
done
for type in kernel_t kmod_t; do
    sesearch --allow -s "$type" -t lib_t -c system -p firmware_load fedora-compat.35 > query
    grep -q 'allow ' query
done
for type in user_t sshd_t; do
    sesearch --allow -s "$type" -t lib_t -c system -p firmware_load fedora-compat.35 > query
    ! grep -q 'allow ' query
done
for type in unconfined_t named_t; do
    sesearch --allow -s "$type" -t "$type" -c io_uring -p allowed fedora-compat.35 > query
    grep -q 'allow ' query
done
sesearch --allow -s sshd_t -t sshd_t -c io_uring -p allowed fedora-compat.35 > query
! grep -q 'allow ' query

# Prove that the parity observer rejects removal of the firmware grant.
sed '/^(allow kernel_t lib_t (system (firmware_load)))$/d' baseline-compat.cil > broken.cil
timeout -k 15s 300s secilc -M true -c 35 -f /dev/null -o broken.35 baseline.cil broken.cil
timeout -k 15s 300s sediff --allow fork.35 broken.35 > "$result/negative-control.txt"
grep -q firmware_load "$result/negative-control.txt"

printf '%s\n' 'Check a synthetic partial original-policy adoption.'
sed -e 's/^(class system (/\0firmware_load /' -e 's/^(class io_uring (/\0allowed /' fedora.cil > partial.cil
printf '%s\n' '(allow named_t self (io_uring (allowed)))' >> partial.cil
gawk -f "$composer" partial.cil > partial-compat.cil
! grep -q firmware_load partial-compat.cil
! grep -q '(io_uring (allowed))' partial-compat.cil
timeout -k 15s 300s secilc -M true -c 35 -f /dev/null -o partial-compat.35 partial.cil partial-compat.cil
sesearch --allow -s named_t -t named_t -c io_uring -p allowed partial-compat.35 > query
grep -q 'allow ' query
sesearch --allow -s unconfined_t -t unconfined_t -c io_uring -p allowed partial-compat.35 > query
! grep -q 'allow ' query
sesearch --allow -s kernel_t -t lib_t -c system -p firmware_load partial-compat.35 > query
! grep -q 'allow ' query

timeout -k 15s 300s sediff --allow fedora.35 fedora-compat.35 > "$result/fedora-delta.txt"
sha256sum baseline-compat.cil fedora-compat.cil already-adopted.cil partial-compat.cil > "$result/compatibility.sha256"
rpm -q checkpolicy secilc libsepol setools-console > "$result/tools.txt"
printf '%s\n' '{"offline_only":true,"reference_rules":108,"historical_decision_parity":true,"fedora45_24_compiles":true,"bounded_grants_and_denials":7,"capabilities_and_permissive_unchanged":true,"negative_control":true,"derived_io_subjects":true,"synthetic_partial_and_full_adoption":true,"normal_dnf_transition":false,"future_base_runtime_adaptation":false,"device_installed":false}' > "$result/result.json"
printf '%s\n' 'Offline class/common composition, derived subjects and historical decision parity passed; no policy loaded. Runtime store and DNF remain unqualified.'
