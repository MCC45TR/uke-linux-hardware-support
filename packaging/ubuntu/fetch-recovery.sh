#!/bin/bash
# Existing Android recovery delivery assets only; never execute or flash them.
set -euo pipefail
root=$(git -C "$(dirname "$0")" rev-parse --show-toplevel)
revision=cbecc441059ebffa97497e736edd2e0b0b0546a5
lock=$(git -C "$root" show "$revision:uke-orangefox-packaging/manifests/release-lock.json")
cache=$root/uke-orangefox-packaging/referances/releases/R12.0-uke-modded-v003
mkdir -p "$cache"
while IFS=$'\t' read -r name url hash bytes; do
 [[ $name =~ ^[A-Za-z0-9][A-Za-z0-9._-]*$ ]]
 if [[ ! -s $cache/$name ]]; then
  curl --fail --location --max-time 180 "$url" -o "$cache/$name.part"
  printf '%s  %s\n' "$hash" "$cache/$name.part" | sha256sum -c -
  [[ $(stat -c %s "$cache/$name.part") == "$bytes" ]]
  mv "$cache/$name.part" "$cache/$name"
 fi
 printf '%s  %s\n' "$hash" "$cache/$name" | sha256sum -c -
 [[ $(stat -c %s "$cache/$name") == "$bytes" ]]
done < <(jq -r '.assets[]|[.name,.url,.sha256,.bytes]|@tsv' <<< "$lock")
