#!/usr/bin/bash
# Extract only the exact installed SSC32 inputs. No donor modification or build.
set -Eeuo pipefail
[[ $# == 3 ]]
source=$(realpath "$1")
output=$(realpath -m "$2")
contract=$(realpath "$3")
[[ ! -e $output && ! -L $output ]]
printf '%s  %s\n' "$(jq -er .ssc.source_archive_sha256 "$contract")" "$source" | sha256sum -c --strict --quiet -
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
tar -tJf "$source" | awk '/^\/|(^|\/)\.\.($|\/)/{bad=1}END{exit bad}'
tar -xJf "$source" -C "$work"
input=$work/uke-sensor-integration-1.0.0/inputs/uke-ssc-fileservice
[[ $(find "$input" -maxdepth 1 -type f | wc -l) == 18 ]]
mkdir "$output"
jq -r '.ssc.inputs[]|.sha256+"  "+.filename' "$contract" | (cd "$input";sha256sum -c --strict --quiet -)
for file in "$input/"*;do
 [[ -f $file && ! -L $file ]]
 install -m644 "$file" "$output/$(basename "$file")"
done
printf 'SSC32:18 original files retained unchanged;legacy protocol kept only until original-provider parity.\n'
