#!/bin/bash
set -euo pipefail
here=$(cd -- "$(dirname -- "$0")" && pwd)
work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT
mkdir -p "$work/host/etc" "$work/host/usr/bin"
printf 'NAME="Ubuntu"\nVERSION="27.04"\nPRETTY_NAME="Ubuntu 27.04"\nID=ubuntu\n' > "$work/host/etc/os-release"
printf 'DRACUT_VERSION="112-7"\n' > "$work/host/usr/bin/dracut"
printf 'NAME="Ubuntu"\nID=ubuntu\nVERSION="27.04 dracut-112-7"\nPRETTY_NAME="Ubuntu 27.04 dracut-112-7 (Initramfs)"\nDRACUT_VERSION="112-7"\n' > "$work/payload"
bash "$here/verify-initrd-release.sh" "$work/payload" "$work/host"
printf 'unexpected-metadata\n' >> "$work/payload"
if bash "$here/verify-initrd-release.sh" "$work/payload" "$work/host" >/dev/null 2>&1; then
 echo 'Modified release metadata was accepted' >&2; exit 1
fi
echo 'Exact generated release metadata accepted; appended data refused.'
