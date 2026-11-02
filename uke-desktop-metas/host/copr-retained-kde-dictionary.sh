#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# COPR source worker only. Retain the installed KDE selection, not a new SDK.
set -Eeuo pipefail
trap 'printf "KDE source preparation failed at line %s.\n" "$LINENO" >&2' ERR
export LC_ALL=C
test ! -e /etc/senemos/kernel-update.conf
[[ $(rpm --eval '%{_host_cpu}') == aarch64 && $(rpm --eval '%{fedora}') == 46 ]]
here=$(cd -- "$(dirname -- "$0")"; pwd)
output=$(realpath -m "${1:?COPR result directory}")
stage=$(mktemp -d)
trap 'rm -rf -- "$stage"' EXIT
for project in uke-linux-unstable uke-linux-test uke-linux; do
    curl -fLsS --max-time 45 --get --data-urlencode ownername=mcc45tr \
        --data-urlencode "projectname=$project" --data-urlencode packagename=uke-control \
        --data-urlencode limit=100 https://copr.fedorainfracloud.org/api_3/build/list > "$stage/builds.json"
    jq -e '(.items|type)=="array" and (.items|length)<100 and all(.items[];
        ((.source_package.version // "")|sub("[.]fc[0-9]+$";""))!="2.8.1-5.1")' "$stage/builds.json" >/dev/null
done
# The signed repository copy differs from COPR's unsigned source-worker copy.
curl -fLsS --proto '=https' --proto-redir '=https' --max-time 180 \
    https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/fedora-rawhide-aarch64/Packages/u/uke-control-2.8.1-5.fc46.src.rpm \
    -o "$stage/parent.src.rpm"
printf '%s  %s\n' 44755d1c58f919bcd76c2a13c6dabefe2cf574464542bfa58fb29309fca5881a "$stage/parent.src.rpm" | sha256sum -c -
curl -fLsS --proto '=https' --proto-redir '=https' --max-time 30 \
    https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/pubkey.gpg -o "$stage/key.asc"
mkdir -m700 "$stage/gpg" "$stage/keys"
fingerprint=$(gpg --homedir "$stage/gpg" --batch --show-keys --with-colons "$stage/key.asc" 2>/dev/null | awk -F: '$1=="fpr"{print $10}')
[[ $fingerprint == 2204A661EA630A5303500085538797E72A3E4ECA ]]
rpmkeys --dbpath "$stage/keys" --import "$stage/key.asc"
rpmkeys --dbpath "$stage/keys" --checksig "$stage/parent.src.rpm"
[[ $(rpm --dbpath "$stage/keys" -qp --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$stage/parent.src.rpm") == uke-control-2.8.1-5.fc46 ]]
rpm2cpio "$stage/parent.src.rpm" > "$stage/input.cpio"
cpio -t --quiet < "$stage/input.cpio" > "$stage/members"
awk '/^\/|(^|\/)\.\.($|\/)/ {bad=1} END{exit bad}' "$stage/members"
[[ $(wc -l < "$stage/members") == 2 ]]
mkdir "$stage/input"
(cd "$stage/input"; cpio -idm --quiet --no-absolute-filenames < "$stage/input.cpio")
test -s "$stage/input/uke-control.spec" -a -s "$stage/input/uke-control-2.8.1.tar.xz"
! grep -q '^Source[1-9]:' "$stage/input/uke-control.spec"
tar -xOf "$stage/input/uke-control-2.8.1.tar.xz" \
    uke-control-2.8.1/inputs/uke-meta-integration/uke-meta-integration.spec > "$stage/meta.spec"
grep '^BuildRequires:' "$stage/meta.spec" > "$stage/buildrequires"
mkdir -p "$output"
cp "$stage/input/uke-control-2.8.1.tar.xz" "$output/"
cp "$stage/input/uke-control.spec" "$output/uke-control-published-spec.txt"
awk -v requirements="$stage/buildrequires" -f "$here/select-retained-kde.awk" \
    "$stage/input/uke-control.spec" > "$output/uke-control.spec"
# Existing KDE package metadata changes only by its release and dictionary.
kde_preamble() { awk '$0=="%package -n uke-kde-plasma-meta"{p=1;next} /^%description/ && p{exit} p{print}' "$1"; }
kde_preamble "$stage/input/uke-control.spec" | sed 's/^Release: 35.fc46$/Release: 35.1.fc46/; /^Requires: glibc-all-langpacks$/a Requires: hunspell-tr' > "$stage/expected-preamble"
kde_preamble "$output/uke-control.spec" > "$stage/actual-preamble"
cmp "$stage/expected-preamble" "$stage/actual-preamble"
sections() { awk '/^%(pre|post|preun|postun|pretrans|posttrans|files)([[:space:]]|$)/{p=($0 ~ /-n uke-kde-plasma-meta$/)} p{print}' "$1"; }
sections "$stage/input/uke-control.spec" > "$stage/sections.before"
sections "$output/uke-control.spec" > "$stage/sections.after"
cmp "$stage/sections.before" "$stage/sections.after"
[[ $(grep -c '^%package ' "$output/uke-control.spec") == 1 ]]
[[ $(grep -c '^%files ' "$output/uke-control.spec") == 1 ]]
[[ $(rpm --eval "%{lua: print(rpm.vercmp('35.1.fc46', '35.fc46'))}") == 1 ]]
[[ $(rpm --eval "%{lua: print(rpm.vercmp('35.1.fc46', '36.fc46'))}") == -1 ]]
[[ $(rpmspec --target aarch64 -q --srpm --qf '%{VERSION}-%{RELEASE}' "$output/uke-control.spec") == 2.8.1-5.1.fc46 ]]
rpmspec --target aarch64 -q --qf '%{NAME}\t%{VERSION}-%{RELEASE}\n' "$output/uke-control.spec" > "$stage/identities"
cat "$stage/identities"
# rpmspec also lists the source-named root even without a %files section.
# The single selected %files section above controls binary production; inspect
# COPR's actual built-package list separately after the native build.
[[ $(wc -l < "$stage/identities") == 2 ]]
grep -Fxq $'uke-control\t2.8.1-5.1.fc46' "$stage/identities"
grep -Fxq $'uke-kde-plasma-meta\t1.0.0-35.1.fc46' "$stage/identities"
commit=$(git -C "$here" rev-parse HEAD)
jq -n --arg commit "$commit" '{source_commit:$commit,parent_copr_build:11240837,
    signed_parent_sha256:"44755d1c58f919bcd76c2a13c6dabefe2cf574464542bfa58fb29309fca5881a",
    source_archive_unchanged:true,kde_scripts_and_file_sections_unchanged:true,
    backend_sdk_and_power_packages_produced:false,original_fedora_turkish_dictionary:true,
    physical_acceptance:false}' > "$output/kde-dictionary-overlay.json"
printf '%s\n' 'Retained KDE-only metadata source and unchanged lifecycle/file sections verified.'
