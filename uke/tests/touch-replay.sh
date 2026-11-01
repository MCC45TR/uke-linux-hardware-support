#!/usr/bin/env bash
set -Eeuo pipefail
[[ $# == 3 ]] || { echo 'Usage: touch-replay.sh REPLAY OEM_CONFIG PRIVATE_CAPTURE' >&2; exit 2; }
replay=$1 config=$2 capture=$3
scratch=$(mktemp -d)
trap 'find "$scratch" -depth -delete' EXIT
dd if="$capture" of="$scratch/reference" bs=5432 count=1 status=none
"$replay" "$scratch/reference" "$config" "$capture" > "$scratch/valid.csv"
touch "$scratch/empty"
if "$replay" "$scratch/reference" "$config" "$scratch/empty"; then exit 1; fi
cp "$scratch/reference" "$scratch/truncated"
printf x >> "$scratch/truncated"
if "$replay" "$scratch/reference" "$config" "$scratch/truncated"; then exit 1; fi
for ((i=0;i<301;i++)); do cat "$scratch/reference"; done > "$scratch/oversized"
if "$replay" "$scratch/reference" "$config" "$scratch/oversized" > "$scratch/oversized.csv"; then exit 1; fi
echo 'PASS: retained capture replay; empty, partial and oversized capture rejection (host only).'
