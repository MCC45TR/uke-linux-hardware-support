#!/usr/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# Offline COPR policy-data qualification. Never load a policy on the host.
set -Eeuo pipefail
export LC_ALL=C
[[ $# == 5 ]]
baseline_srpm=$1
fork_rpm=$2
fedora_rpm=$3
reference=$4
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
cat > definitions.cil <<'CIL'
(common uke_system_compat (firmware_load kexec_image_load kexec_initramfs_load policy_load x509_certificate_load))
(classcommon system uke_system_compat)
(common uke_bpf_compat (map_create_as prog_load_as))
(classcommon bpf uke_bpf_compat)
(common uke_io_uring_compat (allowed))
(classcommon io_uring uke_io_uring_compat)
(class memfd_file (execute_no_trans entrypoint))
(classcommon memfd_file file)
(classorder (unordered memfd_file))
CIL
cat definitions.cil "$reference" > compatibility.cil
[[ $(grep -c '(allow ' "$reference") == 108 ]]
for item in baseline fedora; do
    secilc -M true -c 35 -f /dev/null -o "$item-compat.35" "$item.cil" compatibility.cil
done

# Compare decisions by names, including Boolean conditions and transitions;
# the common layout and permission numbers are deliberately different.
differences=(--allow --auditallow --dontaudit --allowxperm --auditallowxperm
    --dontauditxperm --type_trans --type_change --type_member --role_allow
    --role_trans --range_trans --constrain --mlsconstrain --validatetrans
    --mlsvalidatetrans --initialsid --fs_use --genfscon --netifcon --nodecon
    --portcon --default --property --polcap --typebounds)
sediff "${differences[@]}" fork.35 baseline-compat.35 > "$result/fork-parity.txt"
[[ ! -s $result/fork-parity.txt ]]

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
sed '/^(allow kernel_t lib_t (system (firmware_load)))$/d' compatibility.cil > broken.cil
secilc -M true -c 35 -f /dev/null -o broken.35 baseline.cil broken.cil
sediff --allow fork.35 broken.35 > "$result/negative-control.txt"
grep -q firmware_load "$result/negative-control.txt"

sediff --allow fedora.35 fedora-compat.35 > "$result/fedora-delta.txt"
sha256sum compatibility.cil > "$result/compatibility.sha256"
rpm -q checkpolicy secilc libsepol setools-console > "$result/tools.txt"
printf '%s\n' '{"offline_only":true,"reference_rules":108,"historical_decision_parity":true,"fedora45_24_compiles":true,"bounded_grants_and_denials":7,"capabilities_and_permissive_unchanged":true,"negative_control":true,"normal_dnf_transition":false,"future_base_adaptation":false,"device_installed":false}' > "$result/result.json"
printf '%s\n' 'Offline class/common composition and historical decision parity passed; no policy loaded. DNF and future-base adaptation remain unqualified.'
