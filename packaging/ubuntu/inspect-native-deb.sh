#!/bin/bash
# Read payloads only; never run a packaged executable or maintainer script.
set -euo pipefail
export LC_ALL=C
deb=$(realpath "${1:?DEB required}")
architecture=$(dpkg-deb -f "$deb" Architecture)
[[ $architecture == arm64 || $architecture == all ]]
scratch=$(mktemp -d)
trap 'rm -rf "$scratch"' EXIT
dpkg-deb -x "$deb" "$scratch/payload"
dpkg-deb -e "$deb" "$scratch/control"
if find "$scratch/payload" -type f | grep -E '\.(py|pyc|pyo)$|/python[0-9.]*(/|$)'; then
 echo 'Python payload is not admitted' >&2; exit 1
fi
if dpkg-deb -f "$deb" Depends | grep -E '(^|[, ])python[0-9.-]*([, (]|$)'; then
 echo 'Python runtime dependency is not admitted' >&2; exit 1
fi
if grep -rlIE '^#!.*[/ ]python[0-9.]*([[:space:]]|$)' "$scratch/payload" "$scratch/control"; then
 echo 'Python interpreter script is not admitted' >&2; exit 1
fi
while IFS= read -r -d '' file; do
 # Four magic bytes distinguish ELF from ordinary configuration or firmware.
 if [[ $(od -An -tx1 -N4 "$file" | tr -d ' \n') == 7f454c46 ]]; then
  readelf -h "$file" | grep -E 'Class:.*ELF64'
  readelf -h "$file" | grep -E 'Machine:.*AArch64'
 fi
done < <(find "$scratch/payload" -type f -print0)
printf '%s: architecture and direct payload audit passed\n' "$(basename "$deb")"
