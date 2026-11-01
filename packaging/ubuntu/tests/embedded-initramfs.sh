#!/bin/bash
# Deliberately incomplete data fixtures. No kernel or extracted helper runs.
set -euo pipefail
here=$(cd -- "$(dirname -- "$0")/.." && pwd)
[[ $# == 1 && -f $1 ]]
work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT
printf 'Not a kernel: empty fixture\n' > "$work/empty-image"
if bash "$here/inspect-embedded-initramfs.sh" "$work/empty-image" "$work/empty" > "$work/log" 2>&1; then
 echo 'Empty initramfs was admitted' >&2; exit 1
fi
grep -F 'Exactly one embedded Uke initramfs' "$work/log"
mkdir -p "$work/root/usr/libexec" "$work/root/dev"
cp "$1" "$work/root/usr/libexec/uke-root-guard"
chmod 755 "$work/root/usr/libexec/uke-root-guard"
make_fixture() {
 bsdtar --format=newc -cf "$work/base.cpio" -C "$work/root" .
 if [[ ${1:-none} != none ]]; then
  # Encode metadata directly: never create a device node in the fixture tree.
  trailer=$(grep -aob 'TRAILER!!!' "$work/base.cpio" | cut -d: -f1)
  [[ $trailer =~ ^[0-9]+$ ]]
  dd if="$work/base.cpio" iflag=count_bytes count=$((trailer-110)) status=none > "$work/with-console.cpio"
  printf '070701%08x%08x%08x%08x%08x%08x%08x%08x%08x%08x%08x%08x%08x' 424242 "$1" 0 0 1 0 0 0 0 5 1 12 0 >> "$work/with-console.cpio"
  printf 'dev/console\0\0\0' >> "$work/with-console.cpio"
  dd if="$work/base.cpio" iflag=skip_bytes skip=$((trailer-110)) status=none >> "$work/with-console.cpio"
  mv "$work/with-console.cpio" "$work/base.cpio"
 fi
 printf 'Not a kernel: compressed incomplete fixture\n' > "$work/incomplete-image"
 gzip -n -c "$work/base.cpio" >> "$work/incomplete-image"
 printf 'trailing image data\n' >> "$work/incomplete-image"
}
make_fixture
if bash "$here/inspect-embedded-initramfs.sh" "$work/incomplete-image" "$work/incomplete" > "$work/log" 2>&1; then
 echo 'Missing native handoff was admitted' >&2; exit 1
fi
grep -F 'uke-root-guard: OK' "$work/log"
make_fixture "$((8#20600))"
if bash "$here/inspect-embedded-initramfs.sh" "$work/incomplete-image" "$work/console-valid" > "$work/log" 2>&1; then
 echo 'Missing handoff was admitted with a valid console' >&2; exit 1
fi
grep -F 'uke-root-guard: OK' "$work/log"
[[ ! -e $work/console-valid/dev/console ]]
make_fixture "$((8#20644))"
if bash "$here/inspect-embedded-initramfs.sh" "$work/incomplete-image" "$work/console-invalid" > "$work/log" 2>&1; then
 echo 'Incorrect console permissions were admitted' >&2; exit 1
fi
[[ ! -e $work/console-invalid ]]
echo corruption >> "$work/root/usr/libexec/uke-root-guard"
make_fixture
if bash "$here/inspect-embedded-initramfs.sh" "$work/incomplete-image" "$work/corrupt" > "$work/log" 2>&1; then
 echo 'Changed root guard was admitted' >&2; exit 1
fi
grep -F 'FAILED' "$work/log"
echo 'Empty/incomplete initramfs and changed pinned root guard refused; no target code ran.'
