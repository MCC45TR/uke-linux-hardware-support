#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# COPR %check only. Query binary policies without loading a kernel policy.
set -Eeuo pipefail
export LC_ALL=C
test ! -e /etc/senemos/kernel-update.conf
root=$(realpath -e "${1:?RPM buildroot}")
baseline=$(realpath -e "${2:?original Fedora targeted binary policy}")
[[ $root != / ]]
stage=$(mktemp -d)
trap 'find "$stage" -depth -delete' EXIT
policy=$root/etc/selinux/targeted/policy/policy.35
test -s "$policy"
printf '%s  %s\n' 63ce943b9b060e81b714501cce086bc509f0b9f6457e2fb468824aa35482538c \
    "$baseline" | sha256sum -c -
seinfo --class -x "$baseline" > "$stage/baseline-classes"
! grep -wq memfd_file "$stage/baseline-classes"
! grep -wq firmware_load "$stage/baseline-classes"
! grep -wq map_create_as "$stage/baseline-classes"
for profile in targeted minimum mls; do
    candidate=$root/etc/selinux/$profile/policy/policy.35
    test -s "$candidate"
    seinfo --class -x "$candidate" > "$stage/$profile-classes"
    for permission in memfd_file firmware_load kexec_image_load kexec_initramfs_load \
        policy_load x509_certificate_load map_create_as prog_load_as allowed; do
        grep -wq "$permission" "$stage/$profile-classes"
    done
done
for expression in --polcap --permissive; do
    seinfo "$expression" -x "$baseline" | sort > "$stage/baseline-properties"
    seinfo "$expression" -x "$policy" | sort > "$stage/candidate-properties"
    cmp "$stage/baseline-properties" "$stage/candidate-properties"
done
seinfo --polcap "$policy" > "$stage/capabilities"
! grep -Eq '\b(memfd_class|bpf_token_perms)\b' "$stage/capabilities"

query() {
    sesearch --allow -s "$2" -t "$3" -c "$4" -p "$5" "$1"
}
require_allow() {
    query "$policy" "$@" > "$stage/query"
    grep -Eq '^[[:space:]]*allow ' "$stage/query"
}
require_absent() {
    query "$policy" "$@" > "$stage/query"
    ! grep -Eq '^[[:space:]]*allow ' "$stage/query"
}
require_allow kernel_t lib_t system firmware_load
require_allow kmod_t lib_t system firmware_load
require_absent user_t lib_t system firmware_load
require_absent sshd_t lib_t system firmware_load
require_allow unconfined_t unconfined_t io_uring allowed
require_allow named_t named_t io_uring allowed
require_absent sshd_t sshd_t io_uring allowed

# Compare selected existing permission rules, retaining their conditions while
# ignoring other permissions merged into the same AV rule. This is a bounded
# regression matrix, not a proof of every possible access decision.
view() {
    query "$@" | sed -E \
        's/(:[[:alnum:]_]+)[[:space:]]+(\{[^}]*\}|[[:alnum:]_]+);/\1 PERMISSION;/' \
        | sed '/^[[:space:]]*$/d' | sort -u
}
while read -r source target class permission; do
    view "$baseline" "$source" "$target" "$class" "$permission" > "$stage/old-rule"
    view "$policy" "$source" "$target" "$class" "$permission" > "$stage/new-rule"
    cmp "$stage/old-rule" "$stage/new-rule"
done <<'MATRIX'
kernel_t lib_t file read
kmod_t modules_object_t system module_load
unconfined_t unconfined_t io_uring sqpoll
unconfined_t unconfined_t io_uring override_creds
named_t io_uring_t anon_inode create
sshd_t shadow_t file read
user_t shadow_t file write
user_t lib_t file execute
unconfined_t unconfined_t bpf map_create
kernel_t kernel_t bpf prog_load
sshd_t sshd_t process transition
init_t init_t process setexec
MATRIX
printf '%s\n' 'Three policy profiles, unchanged capabilities/permissive types, seven new-permission cases and twelve existing-permission comparisons passed; no kernel policy loaded.'
