#!/usr/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# Offline COPR policy-data qualification. Never load a policy on the host.
set -Eeuo pipefail
export LC_ALL=C
# Keep policy experiments bounded on the default COPR worker. Resource refusal
# is a failed qualification, never a reason to alter or load the device policy.
ulimit -v 1572864
[[ $# == 6 ]]
baseline_srpm=$1
fork_rpm=$2
fedora_rpm=$3
composer=$4
canonical=$5
result=$6
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
    # Binary decompilation does not retain CIL's attribute-expansion flags.
    # Preserve every existing attribute during this test-only round trip.
    gawk 'match($0,/^\(typeattribute ([[:alnum:]_]+)\)$/,m) {
        print "(expandtypeattribute " m[1] " false)"
    }' "$item.cil" > "$item.keep.cil"
done
[[ $(grep -Ec '^[[:space:]]*\(allow .* \(io_uring \([^)]*allowed' fork.cil) == 100 ]]
[[ $(grep -Ec '^[[:space:]]*\(allow .* \(system \([^)]*firmware_load' fork.cil) == 8 ]]
for item in baseline fedora; do
    gawk -f "$composer" "$item.cil" "$item.cil" > "$item-compat.cil"
    printf 'Compile offline composition: %s\n' "$item"
    timeout -k 15s 300s secilc -M true -c 35 -f /dev/null -o "$item-compat.35" "$item.cil" "$item.keep.cil" "$item-compat.cil"
done
gawk -f "$composer" fork.cil fork.cil > already-adopted.cil
! grep -q '^[[:space:]]*(' already-adopted.cil
gawk -f "$composer" baseline.cil fork.cil > reinstall-compat.cil
cmp baseline-compat.cil reinstall-compat.cil

# Compare the complete decompiled representation without SETools' potentially
# quadratic attribute-pair expansion. Flatten only common permission layout;
# retain names, Boolean branches, label order and all other policy statements.
normalize() {
    timeout -k 15s 300s "$canonical" "$1" < "$2" | sort -u > "$3"
}
compare_records() {
    if ! cmp "$1" "$2"; then
        diff -u "$1" "$2" | awk 'NR<=80{print}' || true
        return 1
    fi
}
printf '%s\n' 'Check the unchanged binary-policy round trip before adapter parity.'
timeout -k 15s 300s secilc -M true -c 35 -f /dev/null -o roundtrip.35 baseline.cil baseline.keep.cil
checkpolicy -b -M -C -o roundtrip.cil roundtrip.35
normalize full baseline.cil original.full
normalize full roundtrip.cil roundtrip.full
compare_records original.full roundtrip.full
printf '%s\n' 'Compare complete historical policy representation.'
checkpolicy -b -M -C -o baseline-linked.cil baseline-compat.35
checkpolicy -b -M -C -o fedora-linked.cil fedora-compat.35
normalize full fork.cil fork.full
normalize full baseline-linked.cil baseline.full
compare_records fork.full baseline.full
printf '%s\n' 'Complete historical normalized representation matches.' > "$result/fork-parity.txt"
normalize old fedora.cil fedora.old
normalize old fedora-linked.cil fedora-compat.old
compare_records fedora.old fedora-compat.old
sha256sum fork.full baseline.full fedora.old fedora-compat.old > "$result/policy-representation.sha256"

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
sed '/^(optional uke_kernel_abi_firmware /d' baseline-compat.cil > broken.cil
timeout -k 15s 300s secilc -M true -c 35 -f /dev/null -o broken.35 baseline.cil baseline.keep.cil broken.cil
checkpolicy -b -M -C -o broken-linked.cil broken.35
normalize full broken-linked.cil broken.full
! cmp -s fork.full broken.full
sesearch --allow -s kernel_t -t lib_t -c system -p firmware_load broken.35 > "$result/negative-control.txt"
! grep -q 'allow ' "$result/negative-control.txt"

printf '%s\n' 'Check a synthetic partial original-policy adoption.'
sed -e 's/^(class system (/\0firmware_load /' -e 's/^(class io_uring (/\0allowed /' fedora.cil > partial.cil
printf '%s\n' '(allow named_t self (io_uring (allowed)))' >> partial.cil
gawk -f "$composer" partial.cil partial.cil > partial-compat.cil
! grep -q firmware_load partial-compat.cil
! grep -q '(io_uring (allowed))' partial-compat.cil
timeout -k 15s 300s secilc -M true -c 35 -f /dev/null -o partial-compat.35 partial.cil fedora.keep.cil partial-compat.cil
sesearch --allow -s named_t -t named_t -c io_uring -p allowed partial-compat.35 > query
grep -q 'allow ' query
sesearch --allow -s unconfined_t -t unconfined_t -c io_uring -p allowed partial-compat.35 > query
! grep -q 'allow ' query
sesearch --allow -s kernel_t -t lib_t -c system -p firmware_load partial-compat.35 > query
! grep -q 'allow ' query

sha256sum baseline-compat.cil fedora-compat.cil already-adopted.cil partial-compat.cil > "$result/compatibility.sha256"
rpm -q checkpolicy secilc libsepol setools-console > "$result/tools.txt"
printf '%s\n' '{"offline_only":true,"reference_rules":108,"historical_normalized_representation_equal":true,"fedora_existing_permission_representation_equal":true,"fedora45_24_compiles":true,"bounded_grants_and_denials":7,"capabilities_and_permissive_unchanged":true,"negative_control":true,"derived_io_subjects":true,"synthetic_partial_and_full_adoption":true,"normal_dnf_transition":false,"future_base_runtime_adaptation":false,"device_installed":false}' > "$result/result.json"
printf '%s\n' 'Offline class/common composition, derived subjects and historical decision parity passed; no policy loaded. Runtime store and DNF remain unqualified.'
