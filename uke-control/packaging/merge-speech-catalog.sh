#!/usr/bin/env bash
# Replace only microphone actions; retain newer package and recovery actions.
set -Eeuo pipefail
template=${1:?Speech catalog template}
target=${2:?Retained catalog}
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
for file in "$template" "$target"; do
 [[ $(grep -c '^        add("microphone.status",' "$file") == 1 ]]
 [[ $(grep -c '^        add("status","overview",' "$file") == 1 ]]
done
awk '/^        add\("microphone.status",/ {copy=1}
 /^        add\("status","overview",/ {copy=0;end=1}
 copy {print} END {exit !end}' "$template" > "$work/microphone"
outside() {
 awk '/^        add\("microphone.status",/ {skip=1}
 /^        add\("status","overview",/ {skip=0}
 !skip {print}' "$1"
}
outside "$target" > "$work/before"
awk -v fragment="$work/microphone" '
 /^        add\("microphone.status",/ {
  while ((getline line < fragment)>0) print line
  close(fragment);skip=1
 }
 /^        add\("status","overview",/ {skip=0}
 !skip {print}' "$target" > "$work/merged"
outside "$work/merged" > "$work/after"
cmp "$work/before" "$work/after"
for action in streams processing.status processing.verify processing.select processing.agc; do
 grep -Fq "add(\"microphone.$action\"," "$work/merged"
done
cat "$work/merged" > "$target"
printf '%s\n' 'Microphone catalog merged; every non-microphone byte retained.'
