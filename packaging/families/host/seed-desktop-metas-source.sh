#!/usr/bin/bash
# Data only: populate the unchanged historical packager's authenticated cache.
set -Eeuo pipefail
[[ $# == 2 ]]
directory=$(realpath "$1")
contract=$(realpath "$2")
verify_reference(){
 while IFS=$'\t' read -r file hash;do
  [[ $file == prepare.sh || $file == input.json || $file == Makefile ]]
  [[ -f $directory/$file && ! -L $directory/$file ]]
  printf '%s  %s\n' "$hash" "$directory/$file" | sha256sum -c --strict --quiet -
 done < <(jq -r '.reference_files[]|[.path,.sha256]|@tsv' "$contract")
}
verify_reference
expected=$(jq -er .native_source.sha256 "$contract")
[[ $expected == "$(jq -er .sha256 "$directory/input.json")" ]]
url=$(jq -er .native_source.archive_url "$contract")
[[ $url == https://github.com/MCC45TR/uke-linux-hardware-support/releases/download/copr-consolidation-archive-20261101/uke-control-2.8.1-5.fc46.native.src.rpm ]]
cache=$directory/referances
[[ ! -L $cache ]]
if [[ ! -e $cache ]];then mkdir -m755 "$cache";fi
[[ -d $cache ]]
file=$cache/uke-control-2.8.1-5.fc46.src.rpm
if [[ -e $file || -L $file ]];then
 [[ -f $file && ! -L $file ]]
 printf '%s  %s\n' "$expected" "$file" | sha256sum -c --strict --quiet -
else
 temporary=$(mktemp "$cache/.source.XXXXXX")
 trap 'rm -f "$temporary"' EXIT
 curl -q --proto '=https' --proto-redir '=https' --tlsv1.2 -fsSL --max-time 90 --max-filesize 8388608 "$url" -o "$temporary"
 printf '%s  %s\n' "$expected" "$temporary" | sha256sum -c --strict --quiet -
 chmod 644 "$temporary"
 mv -T "$temporary" "$file"
 trap - EXIT
fi
verify_reference
printf 'Desktop meta source cache:exact signed44755 input retained;historical prepare/input/Makefile unchanged.\n'
