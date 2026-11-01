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
while IFS=$'\t' read -r package archive url hash member source_hash; do
 if [[ ! -s $root/$archive ]]; then
  mkdir -p "$(dirname "$root/$archive")"
  curl --fail --location --max-time 90 "$url" -o "$root/$archive.part"
  printf '%s  %s\n' "$hash" "$root/$archive.part" | sha256sum -c -
  mv "$root/$archive.part" "$root/$archive"
 fi
 printf '%s  %s\n' "$hash" "$root/$archive" | sha256sum -c -
done < <(cat "$root/packaging/ubuntu/release-sources.tsv" "$root/packaging/ubuntu/release-overrides.tsv" "$root/packaging/ubuntu/external-components.tsv" "$root/packaging/ubuntu/kernel-inputs.tsv")
touch_root=$root/uke-sensors/referances/nt36532-thp
while IFS=$'\t' read -r path hash; do
 if [[ ! -s $touch_root/$path ]]; then
  mkdir -p "$(dirname "$touch_root/$path")"
  curl --fail --location --max-time 90 "https://raw.githubusercontent.com/ianchb/xiaomi-sheng-thp/34046210932d654a4c0df0121ecc31c008f8148c/$path" -o "$touch_root/$path.part"
  printf '%s  %s\n' "$hash" "$touch_root/$path.part" | sha256sum -c -
  mv "$touch_root/$path.part" "$touch_root/$path"
 fi
 printf '%s  %s\n' "$hash" "$touch_root/$path" | sha256sum -c -
done < "$root/packaging/ubuntu/touch-inputs.tsv"
