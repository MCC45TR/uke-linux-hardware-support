#!/bin/sh
# Update standard public identity fields, preserving other machine-info settings.
set -eu
root=${1:-/}
file=$root/etc/machine-info
[ ! -L "$file" ] || { echo 'Refusing a symlink machine-info' >&2; exit 1; }
if [ -e "$file" ]; then
    [ -f "$file" ] && [ "$(stat -c %h "$file")" = 1 ] || exit 1
fi
tmp=$(mktemp "$root/etc/.uke-machine-info.XXXXXX")
trap 'rm -f "$tmp"' EXIT
trap 'exit 1' HUP INT TERM
if [ -f "$file" ]; then
    awk '!/^[[:space:]]*HARDWARE_(VENDOR|MODEL|VERSION)=/' "$file" > "$tmp"
    chmod --reference="$file" "$tmp"
    chown --reference="$file" "$tmp"
else
    chmod 644 "$tmp"
fi
"${root%/}/usr/bin/uke-hardware-info" --root "$root" --machine-info >> "$tmp"
if [ -f "$file" ] && cmp -s "$tmp" "$file"; then exit 0; fi
mv -f "$tmp" "$file"
