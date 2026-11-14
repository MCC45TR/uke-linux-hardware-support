#!/usr/bin/bash
# SPDX-License-Identifier: MIT
# Native COPR payload checks, never execute the installed service on the tablet.
set -Eeuo pipefail
export LC_ALL=C
[[ $# == 3 && ! -e /etc/senemos/kernel-update.conf ]]
source=$(realpath -e -- "$1")
payload=$(realpath -e -- "$2")
spec=$(realpath -e -- "$3")
work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT
for entry in \
 'packaging/uke-control-motion.service usr/lib/systemd/system/uke-control-motion.service' \
 'packaging/org.senemos.Uke.Motion1.service usr/share/dbus-1/system-services/org.senemos.Uke.Motion1.service' \
 'packaging/org.senemos.Uke.Motion1.conf usr/share/dbus-1/system.d/org.senemos.Uke.Motion1.conf' \
 'packaging/org.senemos.uke.motion.policy usr/share/polkit-1/actions/org.senemos.uke.motion.policy' \
 'packaging/selinux/motion-policy.sh usr/libexec/uke-control/motion-policy' \
 'build/motion/selinux/uke_control_motion.cil usr/share/selinux/packages/uke_control_motion.cil' \
 'docs/MOTION1.md usr/share/doc/uke-control/MOTION1.md';do
 read -r input output <<< "$entry"
 cmp "$source/$input" "$payload/$output"
done
# %check runs after Fedora's build-root policy strips debug/comment/LTO data.
# Replay those exact tools on a private copy, retaining a byte-for-byte check
# of the resulting executable instead of comparing the unstripped ELF header.
mkdir "$work/expected"
cp -- "$source/build/motion/motion" "$work/expected/motion"
RPM_BUILD_ROOT="$work/expected" /usr/lib/rpm/brp-strip /usr/bin/strip
RPM_BUILD_ROOT="$work/expected" /usr/lib/rpm/brp-strip-comment-note /usr/bin/strip /usr/bin/objdump
RPM_BUILD_ROOT="$work/expected" /usr/lib/rpm/redhat/brp-strip-lto /usr/bin/strip
cmp "$work/expected/motion" "$payload/usr/libexec/uke-control/motion"
[[ -x $payload/usr/libexec/uke-control/motion && -x $payload/usr/libexec/uke-control/motion-policy ]]
readelf -h "$payload/usr/libexec/uke-control/motion" | grep -Eq 'Machine:[[:space:]]+AArch64'
[[ $(head -n 1 "$payload/usr/share/selinux/packages/uke_control_motion.cil") == '; uke-control managed Motion1 policy v1' ]]
! find "$payload" -type f -name original-fedora-motion-policy.rpm -print | read -r _
# The parsed scriptlet embeds the exact same ownership guard used by the
# tested install helper, so erasure works after its file has been removed.
rpmspec -P "$spec" > "$work/spec"
awk '
 /^UKE_MOTION_POLICY_ERASE$/ {copy=0;done++;next}
 copy {print}
 /<<\047UKE_MOTION_POLICY_ERASE\047$/ {copy=1;found++}
 END{if(found!=1||done!=1)exit 1}' "$work/spec" > "$work/erase-helper"
cmp "$source/packaging/selinux/motion-policy.sh" "$work/erase-helper"
bash -n "$payload/usr/libexec/uke-control/motion-policy"
[[ $(rpmspec -q --qf '%{NAME}\n' "$spec" | sort) == $'uke-control\nuke-control-client-devel\nuke-control-tui' ]]
printf 'Motion payload: native ARM service,exact policy/unit/bus/authorization/docs/erase-helper bytes,three existing package names and no policy fixture leakage.\n'
