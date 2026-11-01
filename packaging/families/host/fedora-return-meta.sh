#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Source-worker metadata overlay; preserve the archived programs and GNU runtime policy.
set -Eeuo pipefail
root=$(cd -- "$(dirname -- "$0")/../../.."; pwd)
input=${1:?archived core selection spec};output=${2:?rewritten spec}
stage=$(mktemp -d)
trap 'rm -rf -- "$stage"' EXIT
grep -Fx 'Name: uke-core-meta' "$input" >/dev/null
grep -Fq 'senemos-native-runtime(libstdc++)' "$input"
grep -Eq '^Requires:.*[[:space:]]uke-libwacom([[:space:]]|$)' "$input"
! grep -Fq 'uke-core-meta(fedora-returns-v1)' "$input"
jq -r '.pairs[]|select(.source_name!="gcc")|. as $pair|"Conflicts: "+.fork_name+" <= "+(.fork.url|split("/")|last|ltrimstr($pair.fork_name+"-"))' \
    "$root/packaging/families/consolidation/fedora-return-inputs.json" > "$stage/raw"
# Use the explicit version and source-family release, without an architecture suffix.
sed -E 's/[.](aarch64|noarch)[.]rpm$//' "$stage/raw" > "$stage/conflicts"
[[ $(wc -l < "$stage/conflicts") == 12 ]]
awk -v conflicts="$stage/conflicts" '
    /^Requires:/ {gsub(/(^|[[:space:]])uke-libwacom([[:space:]]|$)/," libwacom >= 2.20.0 ")}
    /^%description([[:space:]]|$)/ && !inserted {
        print "Requires: uke-control(fedora-returns-v1) >= 1"
        print "Provides: uke-core-meta(fedora-returns-v1) = 1"
        while((getline line < conflicts)>0)print line
        close(conflicts);inserted=1
    }
    {print}
    END {if(!inserted)exit 1}
' "$input" > "$output"
grep -Fq 'senemos-native-runtime(libstdc++)' "$output"
! grep -Eq '^Requires:.*(^|[[:space:]])uke-libwacom([[:space:]]|$)' "$output"
[[ $(grep -c '^Conflicts: uke-' "$output") == 12 ]]
