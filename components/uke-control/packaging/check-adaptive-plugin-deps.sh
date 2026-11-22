#!/usr/bin/bash
# COPR only. A single optional plugin keeps every ELF dependency conditionally.
set -Eeuo pipefail
test ! -e /etc/senemos/kernel-update.conf
[[ $# == 3 ]]
plugin=$(realpath "$1")
contract=$(realpath "$2")
record=$(realpath -m "$3")
[[ -s $plugin && ! -e $record ]]
scanner=''
for path in /usr/lib/rpm/rpmdeps /usr/bin/rpmdeps;do
 if [[ -x $path ]];then scanner=$path;break;fi
done
if [[ -n $scanner ]];then
 "$scanner" --requires "$plugin" | sort -u > "$record"
else
 [[ -x /usr/lib/rpm/elfdeps ]]
 /usr/lib/rpm/elfdeps --requires "$plugin" | sort -u > "$record"
fi
expected=$(mktemp)
trap 'rm -f "$expected"' EXIT
jq -r '.plugin_elf_requires[]' "$contract" | sort -u > "$expected"
[[ -s $record ]]
cmp "$expected" "$record"
printf 'Adaptive plugin dependency coverage: every actual native ELF requirement matches the explicit consumer-conditional set.\n'
