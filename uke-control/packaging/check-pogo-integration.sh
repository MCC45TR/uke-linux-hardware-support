#!/usr/bin/bash
# COPR only; retain original protocol/credential and desktop state fixtures.
set -Eeuo pipefail
[[ ! -e /etc/senemos/kernel-update.conf && $# == 2 ]]
root=$(realpath "$1")
payload=$(realpath "$2")
bash "$root/auth/uke-pogo-auth/tests/check.sh" "$root/auth/upstream/minkipc"
bash "$root/auth/uke-pogo-auth/tests/credentials.sh" "$root/auth/upstream"
bash "$root/desktop/tests/check.sh"
bash "$root/tests/pogo-runtime.sh" "$root"
bash "$root/tests/pogo-ownership.sh" "$root"
cmp "$root/src/pogo-runtime.hpp" "$root/auth/uke-pogo-auth/src/control-pogo-runtime.hpp"
cmp "$root/desktop/src/state.hpp" "$root/auth/uke-pogo-auth/src/control-pogo-state.hpp"
for file in uke-pogo-auth uke-pogo-desktop;do
 readelf -h "$payload/usr/libexec/$file" | grep -Fq AArch64
done
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
for component in auth desktop;do
 binary=uke-pogo-$component
 objcopy --dump-section ".text=$work/built-text" "$root/$component/build/$binary" "$work/built-copy"
 objcopy --dump-section ".text=$work/installed-text" "$payload/usr/libexec/$binary" "$work/installed-copy"
 cmp "$work/built-text" "$work/installed-text"
done
# MINK is still needed: Fedora currently packages only its IDL compiler.
# Its original public QCOMTEE interfaces must match the accepted source pin.
for header in qcomtee_errno.h qcomtee_object.h qcomtee_object_types.h;do
 cmp "$root/auth/upstream/quic-teec/libqcomtee/include/$header" "/usr/include/$header"
done
readelf -d "$payload/usr/libexec/uke-pogo-auth" > "$work/auth-dynamic"
grep -Fq 'Shared library: [libqcomtee.so.1]' "$work/auth-dynamic"
nm -D "$payload/usr/libexec/uke-pogo-auth" > "$work/auth-symbols"
grep -Eq ' U qcomtee_object_invoke(@|$)' "$work/auth-symbols"
! grep -Eq ' [TW] qcomtee_object_' "$work/auth-symbols"
gcc -std=gnu11 -Wall -Wextra -Werror -UNDEBUG \
 "$root/tests/pogo-fedora-credentials.c" $(pkg-config --cflags --libs qcomtee libcbor) -o "$work/fedora-credentials"
strace -qq -e trace=openat,ioctl -P /dev/tee0 -o "$work/credentials.trace" "$work/fedora-credentials"
[[ ! -s $work/credentials.trace ]]
jq -r '.optional_desktop_requires[]' "$root/pogo-services.json" | sort -u > "$work/expected"
printf '%s\n' "$payload/usr/libexec/uke-pogo-desktop" | /usr/lib/rpm/find-requires | sort -u > "$work/actual"
diff -u "$work/expected" "$work/actual"
rc=0
strace -qq -e trace=openat,ioctl -P /dev/tee0 -o "$work/condition.trace" \
 "$payload/usr/libexec/uke-pogo-auth" --bridge-attached || rc=$?
[[ $rc == 255 ]]
[[ ! -s $work/condition.trace ]]
grep -Fq 'uke_pogo_runtime::published_unstable_release(release)' "$root/auth/uke-pogo-auth/src/main.cpp"
grep -Fq 'bridge.wait(stop_channel.descriptor())' "$root/auth/uke-pogo-auth/src/main.cpp"
! grep -Eq 'sleep_for|poll\(&event,1,1000\)' "$root/auth/uke-pogo-auth/src/main.cpp"
grep -Fxq 'ExecCondition=/usr/libexec/uke-pogo-auth --bridge-attached' \
 "$payload/usr/lib/systemd/system/uke-pogo-auth.service.d/pogo-attachment.conf"
udevadm verify --resolve-names=never "$payload/usr/lib/udev/rules.d/70-uke-pogo.rules"
for notice in MIT-LICENSE.txt minkipc-LICENSE.txt quic-teec-LICENSE.txt qcbor-LICENSE.txt LINARO-tee-interface-notice.h DESKTOP-LICENSE.txt;do
 [[ -s $payload/usr/share/licenses/uke-control/pogo/$notice ]]
done
for doc in POGO-AUTH.md POGO-DESKTOP.md POGO-INTEGRATION.md;do
 [[ -s $payload/usr/share/doc/uke-control/$doc ]]
done
printf 'Pogo protocol,original Fedora credential callback,desktop,blocking stop events,ownership,conditional ELF dependencies and no-TEE checks pass;physical acceptance remains open.\n'
