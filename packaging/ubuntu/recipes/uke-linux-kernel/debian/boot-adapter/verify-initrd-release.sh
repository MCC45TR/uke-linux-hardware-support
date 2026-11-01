#!/bin/bash
# Compare generated release metadata; never source the target payload.
set -euo pipefail
[[ $# == 2 && -f $1 ]]
payload=$1
host=${2%/}
[[ -f $host/etc/os-release && -f $host/usr/bin/dracut ]]
version=$(sed -n 's/^DRACUT_VERSION="\([^"]*\)"$/\1/p' "$host/usr/bin/dracut")
[[ $version =~ ^[0-9]+([.+~-][0-9A-Za-z]+)*$ ]]
expected=$(mktemp)
trap 'rm -f -- "$expected"' EXIT
# Only the official build-host os-release is sourced. Its signed package
# provenance is retained separately; the generated payload is never executed.
VERSION= PRETTY_NAME=
source "$host/etc/os-release"
grep -hE -ve '^VERSION=' -ve '^PRETTY_NAME' "$host/etc/os-release" > "$expected"
[[ -z $VERSION ]] || VERSION+=' '
[[ -z $PRETTY_NAME ]] || PRETTY_NAME+=' '
printf 'VERSION="%sdracut-%s"\nPRETTY_NAME="%sdracut-%s (Initramfs)"\nDRACUT_VERSION="%s"\n' "$VERSION" "$version" "$PRETTY_NAME" "$version" "$version" >> "$expected"
cmp "$expected" "$payload"
