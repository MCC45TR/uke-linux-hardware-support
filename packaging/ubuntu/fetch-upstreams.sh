#!/bin/bash
# Download reviewed source bytes only; never execute downloaded content.
set -euo pipefail
root=$(git -C "$(dirname "$0")" rev-parse --show-toplevel)
while IFS=$'\t' read -r package archive url hash patchset; do
 if [[ ! -s $root/$archive ]]; then
  mkdir -p "$(dirname "$root/$archive")"
  curl --fail --location --max-time 90 "$url" -o "$root/$archive.part"
  printf '%s  %s\n' "$hash" "$root/$archive.part" | sha256sum -c -
  mv "$root/$archive.part" "$root/$archive"
 fi
 printf '%s  %s\n' "$hash" "$root/$archive" | sha256sum -c -
done < "$root/packaging/ubuntu/upstreams.tsv"
