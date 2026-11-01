#!/usr/bin/env bash
# COPR source worker only: signed real RPMs and actual Fedora dependency metadata.
set -Eeuo pipefail
[[ ${UKE_COPR_INTEGRATION_FIXTURE:-} == 1 ]]
root=$(cd -- "$(dirname -- "$0")/../../.."; pwd)
fixture=$(mktemp -d)
trap 'rm -rf -- "$fixture"' EXIT
mkdir -p "$fixture"/{baseline,candidate,keys,roots,cache}
touch "$fixture/empty.conf"
archive=https://github.com/MCC45TR/uke-linux-hardware-support/releases/download/copr-consolidation-archive-20261101
curl --fail --location --silent --show-error --max-time 120 "$archive/SHA256SUMS" -o "$fixture/SHA256SUMS"
curl --fail --location --silent --show-error --max-time 60 "$archive/copr-pubkey.gpg" -o "$fixture/copr-pubkey.gpg"
(cd "$fixture"; rg '  copr-pubkey.gpg$' SHA256SUMS | sha256sum -c -)
rpmkeys --dbpath "$fixture/keys" --import "$fixture/copr-pubkey.gpg"
rpmkeys --dbpath "$fixture/keys" --list | grep -qi '^2204a661ea630a5303500085538797e72a3e4eca '
while read -r hash file; do
    [[ $file == *.rpm && $file != *.src.rpm ]] || continue
    curl --fail --location --silent --show-error --max-time 120 "$archive/${file#./}" -o "$fixture/baseline/${file#./}"
    printf '%s  %s\n' "$hash" "$fixture/baseline/${file#./}" | sha256sum -c -
    rpmkeys --dbpath "$fixture/keys" --checksig "$fixture/baseline/${file#./}"
done < "$fixture/SHA256SUMS"
while IFS=$'\t' read -r id group; do
    base="https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/fedora-rawhide-aarch64/$id-$group/"
    curl --fail --location --silent --show-error --max-time 60 "${base}results.json" -o "$fixture/results.json"
    while IFS= read -r file; do
        curl --fail --location --silent --show-error --max-time 120 "$base$file" -o "$fixture/candidate/$file"
        rpmkeys --dbpath "$fixture/keys" --checksig "$fixture/candidate/$file"
    done < <(jq -r '.packages[]|select(.arch!="src")|.name+"-"+.version+"-"+.release+"."+.arch+".rpm"' "$fixture/results.json")
done < <(jq -r '.builds[]|[.id,.name]|@tsv' "$root/packaging/families/consolidation/builds.json")
rpm -qp --qf '%{NAME}\n' "$fixture/baseline/"*.rpm | LC_ALL=C sort > "$fixture/old-names"
rpm -qp --qf '%{NAME}\n' "$fixture/candidate/"*.rpm | LC_ALL=C sort > "$fixture/new-names"
cmp "$fixture/old-names" "$fixture/new-names"
rg -v -- '-(debuginfo|debugsource)$' "$fixture/new-names" > "$fixture/runtime-names"
for repo in baseline candidate; do createrepo_c --quiet "$fixture/$repo"; done
c++ -std=c++20 -O2 -Wall -Wextra -Werror $(pkg-config --cflags libdnf5) "$root/packaging/families/tests/integration-solver.cpp" -o "$fixture/solver" $(pkg-config --libs libdnf5)
for selection in core kde all; do
    for mode in fresh upgrade mixed rollback weak-off; do
        target="$fixture/roots/$selection-$mode"
        mkdir -p "$target/usr/lib/sysimage/rpm"
        rpm --dbpath "$target/usr/lib/sysimage/rpm" --initdb
        case $mode in
            upgrade) seed=("$fixture/baseline/"*.rpm);;
            rollback) seed=("$fixture/candidate/"*.rpm);;
            mixed)
                seed=();i=0
                while IFS= read -r name; do
                    ((i+=1));repo=baseline;((i%2)) || repo=candidate
                    mapfile -t match < <(rpm -qp --qf '%{NAME}\t%{VERSION}-%{RELEASE}.%{ARCH}\n' "$fixture/$repo/$name-"*.rpm | awk -F '\t' -v n="$name" '$1==n{print $2}')
                    [[ ${#match[@]} == 1 ]];seed+=("$fixture/$repo/$name-${match[0]}.rpm")
                done < "$fixture/runtime-names";;
            *) seed=();;
        esac
        if ((${#seed[@]})); then rpm --dbpath "$target/usr/lib/sysimage/rpm" -i --justdb --nodeps --noscripts --notriggers "${seed[@]}"; fi
        "$fixture/solver" "$fixture" "$target" "$mode" "$selection"
    done
done
