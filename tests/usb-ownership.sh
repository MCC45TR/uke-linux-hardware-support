#!/usr/bin/env bash
# Offline cleanup refusal fixtures: no configfs, module or service operations.
set -Eeuo pipefail
here=$(cd -- "$(dirname -- "$0")/.." && pwd)
work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT
state=$work/state
g=$work/gadget
mkdir -p "$state" "$g"/functions/acm.gs0 "$g"/configs/c.1
stat -c '%d:%i' "$g" > "$state/usb-gadget-owner"
printf 'auto-acm\n' > "$state/usb-gadget-profile"
printf 'recorded-controller\n' > "$g/UDC"
sed -n '/^owned() {/,/^profile=network/p' "$here/src/uke-usb-network.sh" | sed '$d' > "$work/functions.sh"
source "$work/functions.sh"
refused_without_unbind() {
    if cleanup; then echo 'Unexpected cleanup admission' >&2; exit 1; fi
    [[ $(cat "$g/UDC") == recorded-controller ]]
    [[ -d $g/functions/acm.gs0 && -f $state/usb-gadget-owner ]]
}
mkdir "$g/functions/foreign.0"
refused_without_unbind
rmdir "$g/functions/foreign.0"
mkdir "$g/configs/foreign"
refused_without_unbind
rmdir "$g/configs/foreign"
ln -s "$work/elsewhere" "$g/configs/c.1/acm.gs0"
refused_without_unbind
rm "$g/configs/c.1/acm.gs0"
ln -s "$g/functions/acm.gs0" "$g/configs/c.1/foreign.0"
refused_without_unbind
rm "$g/configs/c.1/foreign.0"
printf 'manual-acm\n' > "$state/usb-gadget-profile"
mkdir "$g/functions/ecm.usb0"
refused_without_unbind
rmdir "$g/functions/ecm.usb0"
printf 'wrong-inode\n' > "$state/usb-gadget-owner"
refused_without_unbind
printf 'USB foreign ownership/layout refused before unbind; offline only\n'
