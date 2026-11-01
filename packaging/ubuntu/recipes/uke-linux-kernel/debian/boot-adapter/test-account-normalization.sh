#!/bin/bash
set -euo pipefail
here=$(cd -- "$(dirname -- "$0")" && pwd)
work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT
mkdir -p "$work/etc"
printf 'root:x:0:0:root:/root:/bin/sh\n' > "$work/etc/passwd"
printf 'root:x:0:\n' > "$work/etc/group"
before=$(sha256sum "$work/etc/passwd" "$work/etc/group")
printf 'root:!*:20758:0:99999:7:::\n' > "$work/etc/shadow"
printf 'root:!*::\n' > "$work/etc/gshadow"
bash "$here/normalize-initramfs-accounts.sh" "$work"
[[ ! -e $work/etc/shadow && ! -e $work/etc/gshadow ]]
[[ $(sha256sum "$work/etc/passwd" "$work/etc/group") == "$before" ]]
printf 'root:!unprovisioned:20758::::::\n' > "$work/etc/shadow"
bash "$here/normalize-initramfs-accounts.sh" "$work"
[[ ! -e $work/etc/shadow ]]
for entry in 'root:$6$synthetic-test-only:20758:0:99999:7:::' 'root::20758:0:99999:7:::' 'unexpected:!*:20758:0:99999:7:::'; do
 printf '%s\n' "$entry" > "$work/etc/shadow"
 if bash "$here/normalize-initramfs-accounts.sh" "$work"; then
  echo 'Credential/unknown-account refusal failed' >&2; exit 1
 fi
 [[ -f $work/etc/shadow ]]
done
printf 'Locked generated tables omitted; password hashes, empty passwords and unknown accounts refused.\n'
