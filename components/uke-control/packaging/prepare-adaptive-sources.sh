#!/usr/bin/bash
# Data only: reuse the authenticated Adaptive17 inputs already in Source0.
set -Eeuo pipefail
[[ $# == 3 ]]
input=$(realpath "$1")
output=$(realpath -m "$2")
contract=$(realpath "$3")
[[ ! -e $output && ! -L $output ]]
extract(){
 local file=$1 expected=$2 prefix=$3 destination=$4
 [[ -f $file && ! -L $file ]]
 printf '%s  %s\n' "$expected" "$file" | sha256sum -c --strict --quiet -
 tar -tf "$file" | awk -v prefix="$prefix/" '
  /^\/|(^|\/)\.\.($|\/)/{bad=1}
  index($0,prefix)!=1{bad=1}
  END{exit bad}'
 mkdir "$destination"
 tar -xf "$file" --strip-components=1 -C "$destination"
 [[ -z $(find "$destination" -type l -print -quit) ]]
}
mkdir "$output"
extract "$input/inputs/uke-power-integration/uke-power-integration-1.0.0.tar.xz" \
 "$(jq -er .archives.power "$contract")" uke-power-integration-1.0.0 "$output/power"
(cd "$output/power";sha256sum -c --strict --quiet SOURCE-SHA256SUMS)
extract "$output/power/inputs/uke-adaptive-display/uke-adaptive-display-1.0.16.tar.xz" \
 "$(jq -er .archives.wrapper "$contract")" uke-adaptive-display-1.0.16 "$output/wrapper"
(cd "$output/wrapper";sha256sum -c --strict --quiet SOURCE-SHA256SUMS)
extract "$output/wrapper/inputs/uke-adaptive-display/uke-adaptive-display-1.0.16.tar.gz" \
 "$(jq -er .archives.runtime "$contract")" uke-adaptive-display-1.0.16 "$output/runtime"
[[ $(find "$output/runtime" -type f | wc -l) == "$(jq -er '.runtime_files|length' "$contract")" ]]
(cd "$output/runtime";jq -r '.runtime_files[]|.sha256+"  "+.path' "$contract" | sha256sum -c --strict --quiet -)
printf 'Adaptive17 source: three exact nested archives, complete runtime inventory and original source seals pass; no program compiled or run.\n'
