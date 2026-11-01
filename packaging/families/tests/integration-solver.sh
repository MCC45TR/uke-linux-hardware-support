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
(cd "$fixture"; grep '  copr-pubkey.gpg$' SHA256SUMS | sha256sum -c -)
rpmkeys --dbpath "$fixture/keys" --import "$fixture/copr-pubkey.gpg"
rpmkeys --dbpath "$fixture/keys" --list | grep -qi '^2204a661ea630a5303500085538797e72a3e4eca '
fetch() {
    local id=$1 group=$2 repo=$3 base="https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/fedora-rawhide-aarch64/$1-$2/"
    curl -fLsS --max-time 60 "${base}results.json" -o "$fixture/results.json"
    while IFS= read -r file;do
        [[ $file =~ ^[A-Za-z0-9+_.~^-]+[.]rpm$ ]]
        curl -fLsS --max-time 240 "$base$file" -o "$fixture/$repo/$file"
        rpmkeys --dbpath "$fixture/keys" --checksig "$fixture/$repo/$file"
    done < <(jq -r '.packages[]|select(.arch!="src" and (.name|test("-debuginfo$|-debugsource$")|not))|.name+"-"+.version+"-"+.release+"."+.arch+".rpm"' "$fixture/results.json")
}
if [[ -n ${UKE_COPR_CONTROL_FAMILY_BUILD:-} ]];then
    [[ $UKE_COPR_CONTROL_FAMILY_BUILD =~ ^[0-9]+$ ]]
    fetch 11239677 uke-control baseline
    fetch 11239896 uke-meta-integration baseline
    fetch 11239842 uke-power-integration baseline
    mkdir "$fixture/kernel"
    fetch 11238013 senemos-uke-linux-kernel-mainline kernel
    [[ $(find "$fixture/kernel" -maxdepth 1 -name '*.rpm' | wc -l) == 5 ]]
else
while read -r hash file; do
    file=${file#./}
    [[ $file == *.rpm && $file != *.src.rpm ]] || continue
    [[ $hash =~ ^[a-f0-9]{64}$ && $file =~ ^[A-Za-z0-9+_.~^-]+[.]rpm$ ]]
    curl --fail --location --silent --show-error --max-time 120 "$archive/${file#./}" -o "$fixture/baseline/${file#./}"
    printf '%s  %s\n' "$hash" "$fixture/baseline/${file#./}" | sha256sum -c -
    rpmkeys --dbpath "$fixture/keys" --checksig "$fixture/baseline/${file#./}"
done < "$fixture/SHA256SUMS"
fi
while IFS=$'\t' read -r id group; do
    fetch "$id" "$group" candidate
done < <(if [[ -n ${UKE_COPR_CONTROL_FAMILY_BUILD:-} ]];then printf '%s\tuke-control\n' "$UKE_COPR_CONTROL_FAMILY_BUILD";else jq -r '.builds[]|[.id,.name]|@tsv' "$root/packaging/families/consolidation/builds.json";fi)
rpm -qp --qf '%{NAME}\n' "$fixture/baseline/"*.rpm | LC_ALL=C sort > "$fixture/old-names"
rpm -qp --qf '%{NAME}\n' "$fixture/candidate/"*.rpm | LC_ALL=C sort > "$fixture/new-names"
cmp "$fixture/old-names" "$fixture/new-names"
grep -Ev -- '-(debuginfo|debugsource)$' "$fixture/new-names" > "$fixture/runtime-names"
for repo in baseline candidate; do createrepo_c --quiet "$fixture/$repo"; done
c++ -std=c++20 -O2 -Wall -Wextra -Werror $(pkg-config --cflags libdnf5) "$root/packaging/families/tests/integration-solver.cpp" -o "$fixture/solver" $(pkg-config --libs libdnf5)
selections=(core kde all)
[[ -z ${UKE_COPR_CONTROL_FAMILY_BUILD:-} ]] || selections+=(control)
for selection in "${selections[@]}"; do
    for mode in fresh upgrade mixed rollback weak-off; do
        target="$fixture/roots/$selection-$mode"
        mkdir -p "$target/usr/lib/sysimage/rpm"
        rpm --dbpath "$target/usr/lib/sysimage/rpm" --initdb
        rpmkeys --dbpath "$target/usr/lib/sysimage/rpm" --import "$fixture/copr-pubkey.gpg"
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
        [[ -z ${UKE_COPR_CONTROL_FAMILY_BUILD:-} ]] || seed+=("$fixture/kernel/"*.rpm)
        # The separate Cutefish repository is not admitted by this Rawhide fixture.
        for i in "${!seed[@]}"; do [[ ${seed[$i]##*/} != uke-cutefish-meta-* ]] || unset 'seed[i]'; done
        # RPM 6 runs its built-in sysusers hook separately from --noscripts.
        if ((${#seed[@]})); then rpm --define '__systemd_sysusers /usr/bin/true' --dbpath "$target/usr/lib/sysimage/rpm" -i --justdb --nodeps --noscripts --notriggers "${seed[@]}"; fi
        "$fixture/solver" "$fixture" "$target" "$mode" "$selection"
    done
done

# Exercise RPM's real config handling without chroot privileges or running scriptlets.
while IFS= read -r name; do
    old=$(find "$fixture/baseline" -maxdepth 1 -name "$name-*.rpm" | while IFS= read -r file; do [[ $(rpm -qp --qf '%{NAME}' "$file") != "$name" ]] || printf '%s\n' "$file"; done)
    new=$(find "$fixture/candidate" -maxdepth 1 -name "$name-*.rpm" | while IFS= read -r file; do [[ $(rpm -qp --qf '%{NAME}' "$file") != "$name" ]] || printf '%s\n' "$file"; done)
    mapfile -t configs < <(rpm -qp --qf '[%{FILENAMES}\t%{FILEFLAGS:fflags}\n]' "$old" | awk -F '\t' '$2~/c/ && $2~/n/{print $1}')
    ((${#configs[@]})) || continue
    sandbox="$fixture/roots/config-$name"
    mkdir -p "$sandbox/db" "$sandbox/payload"
    rpm --dbpath "$sandbox/db" --initdb
    rpmkeys --dbpath "$sandbox/db" --import "$fixture/copr-pubkey.gpg"
    args=(--define '__systemd_sysusers /usr/bin/true' --dbpath "$sandbox/db" --nodeps --noscripts --notriggers --badreloc --relocate "/=$sandbox/payload")
    rpm "${args[@]}" -i "$old"
    for config in "${configs[@]}"; do
        [[ $config == /* && $config != *..* && -f $sandbox/payload$config && ! -L $sandbox/payload$config ]]
        printf 'Uke migration fixture modified configuration\n' > "$sandbox/payload$config"
    done
    rpm "${args[@]}" -U "$new"
    for config in "${configs[@]}"; do grep -Fxq 'Uke migration fixture modified configuration' "$sandbox/payload$config"; done
    rpm "${args[@]}" -U --oldpackage "$old"
    for config in "${configs[@]}"; do grep -Fxq 'Uke migration fixture modified configuration' "$sandbox/payload$config"; done
    rpm --define '__systemd_sysusers /usr/bin/true' --dbpath "$sandbox/db" --nodeps --noscripts --notriggers -e "$name"
    for config in "${configs[@]}"; do grep -Fxq 'Uke migration fixture modified configuration' "$sandbox/payload$config.rpmsave"; done
    printf 'UKE_INTEGRATION_CONFIG package=%s configs=%s upgrade=true rollback=true rpmsave=true scriptlets_executed=false hardware_tested=false\n' "$name" "${#configs[@]}"
done < "$fixture/runtime-names"
