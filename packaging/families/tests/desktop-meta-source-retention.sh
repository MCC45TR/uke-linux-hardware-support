#!/usr/bin/bash
# Data only; never execute the historical source generator or any RPM payload.
set -Eeuo pipefail
[[ $# == 2 ]]
reference=$(realpath "$1")
output=$(realpath -m "$2")
root=$(cd -- "$(dirname -- "$0")/../../.." && pwd)
helper=$root/packaging/families/host/seed-desktop-metas-source.sh
contract=$root/packaging/families/consolidation/desktop-metas-source-retention.json
[[ ! -e $output && ! -L $output ]]
mkdir "$output" "$output/valid"
for file in prepare.sh input.json Makefile;do cp "$reference/$file" "$output/valid/$file";done
(cd "$output/valid";sha256sum prepare.sh input.json Makefile) > "$output/reference.sha256"
bash "$helper" "$output/valid" "$contract"
bash "$helper" "$output/valid" "$contract"
(cd "$output/valid";sha256sum -c --strict --quiet "$output/reference.sha256")
for mode in foreign symlink stale;do
 mkdir "$output/$mode"
 for file in prepare.sh input.json Makefile;do cp "$reference/$file" "$output/$mode/$file";done
 case $mode in
  foreign) printf '\n' >> "$output/foreign/input.json";;
  symlink) mkdir "$output/foreign-cache";ln -s "$output/foreign-cache" "$output/symlink/referances";;
  stale) mkdir "$output/stale/referances";printf invalid > "$output/stale/referances/uke-control-2.8.1-5.fc46.src.rpm";;
 esac
 if bash "$helper" "$output/$mode" "$contract" > "$output/$mode.log" 2>&1;then exit 1;fi
done
[[ ! -e $output/foreign/referances && -z $(find "$output/foreign-cache" -mindepth 1 -print -quit) ]]
[[ $(cat "$output/stale/referances/uke-control-2.8.1-5.fc46.src.rpm") == invalid ]]
printf 'Desktop meta cache checks:exact signed input/idempotence,unchanged reference code and foreign/symlink/stale refusals pass;no source generator,native build or RPM execution.\n'
