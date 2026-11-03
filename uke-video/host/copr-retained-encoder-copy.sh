#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# COPR source worker only; retain the exact signed installed integration.
set -Eeuo pipefail
export LC_ALL=C
test ! -e /sys/firmware/devicetree/base/model
[[ $(rpm --eval '%{_host_cpu}') == aarch64 && $(rpm --eval '%{fedora}') == 46 ]]
here=$(cd -- "$(dirname -- "$0")"; pwd)
output=$(realpath -m "${1:?COPR result directory}")
stage=$(mktemp -d)
trap 'rm -rf -- "$stage"' EXIT

curl -fLsS --max-time 40 \
    'https://copr.fedorainfracloud.org/api_3/build/list?ownername=mcc45tr&projectname=uke-linux-unstable&packagename=uke-video-integration&limit=100' \
    -o "$stage/history.json"
jq -e '(.items|type)=="array" and (.items|length)<100 and all(.items[];
    ((.source_package.version // "")|sub("[.]fc[0-9]+$";""))!="1.0.0-2")' \
    "$stage/history.json" >/dev/null

curl -fLsS --proto '=https' --proto-redir '=https' --max-time 90 \
    https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/fedora-rawhide-aarch64/Packages/u/uke-video-integration-1.0.0-1.fc46.src.rpm \
    -o "$stage/parent.src.rpm"
printf '%s  %s\n' 75fc93dda6d9f5543d8d3e73e9a4bba2bedb1b097d361ff13fedfab327bdea6c "$stage/parent.src.rpm" | sha256sum -c -
curl -fLsS --proto '=https' --proto-redir '=https' --max-time 30 \
    https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/pubkey.gpg \
    -o "$stage/key.asc"
mkdir -m700 "$stage/gpg" "$stage/keys"
fingerprint=$(gpg --homedir "$stage/gpg" --batch --show-keys --with-colons "$stage/key.asc" 2>/dev/null | awk -F: '$1=="fpr"{print $10}')
[[ $fingerprint == 2204A661EA630A5303500085538797E72A3E4ECA ]]
rpmkeys --dbpath "$stage/keys" --import "$stage/key.asc"
rpmkeys --dbpath "$stage/keys" --checksig "$stage/parent.src.rpm"
[[ $(rpm -qp --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$stage/parent.src.rpm") == uke-video-integration-1.0.0-1.fc46 ]]

rpm2cpio "$stage/parent.src.rpm" > "$stage/input.cpio"
cpio -t --quiet < "$stage/input.cpio" > "$stage/members"
awk '/^\/|(^|\/)\.\.($|\/)/ {bad=1} END{exit bad}' "$stage/members"
[[ $(wc -l < "$stage/members") == 2 ]]
mkdir "$stage/input" "$output"
(cd "$stage/input"; cpio -idm --quiet --no-absolute-filenames < "$stage/input.cpio")
cp "$stage/input/uke-video-integration-1.0.0.tar.xz" "$output/"
cp "$stage/input/uke-video-integration.spec" "$output/uke-video-integration-published-spec.txt"
cp "$here/../patches/0008-encoder-cpu-copy-default.patch" "$output/"
cp "$here/ENCODER-COPY-FIX.md" "$output/"
awk -f "$here/retained-encoder-copy.awk" "$stage/input/uke-video-integration.spec" > "$output/uke-video-integration.spec"
cmp "$stage/input/uke-video-integration-1.0.0.tar.xz" "$output/uke-video-integration-1.0.0.tar.xz"
grep '^Requires:' "$stage/input/uke-video-integration.spec" > "$stage/requires.before"
grep '^Requires:' "$output/uke-video-integration.spec" > "$stage/requires.after"
cmp "$stage/requires.before" "$stage/requires.after"
scriptlets() { awk '/^%(pre|post|preun|postun|pretrans|posttrans)([[:space:]]|$)/{p=1} /^%(files|changelog)([[:space:]]|$)/{p=0} p{print}' "$1"; }
scriptlets "$stage/input/uke-video-integration.spec" > "$stage/scripts.before"
scriptlets "$output/uke-video-integration.spec" > "$stage/scripts.after"
cmp "$stage/scripts.before" "$stage/scripts.after"
[[ $(grep -c '^%package ' "$output/uke-video-integration.spec") == 2 ]]
[[ $(grep -c '^%files ' "$output/uke-video-integration.spec") == 2 ]]
rpmspec --target aarch64 -q --qf '%{NAME}\t%{VERSION}-%{RELEASE}\n' "$output/uke-video-integration.spec" > "$stage/identities"
cat "$stage/identities"
grep -Fxq $'uke-video-integration\t1.0.0-2.fc46' "$stage/identities"
grep -Fxq $'uke-vaapi-v4l2\t1.0.0-12.fc46' "$stage/identities"
grep -Fxq $'uke-video-session\t1.1.0-3.fc46' "$stage/identities"
[[ $(wc -l < "$stage/identities") == 3 ]]
commit=$(git -C "$here" rev-parse HEAD)
jq -n --arg commit "$commit" '{source_commit:$commit,parent_copr_build:11239487,
    signed_parent_sha256:"75fc93dda6d9f5543d8d3e73e9a4bba2bedb1b097d361ff13fedfab327bdea6c",
    retained_archive_unchanged:true,runtime_dependencies_unchanged:true,
    lifecycle_scripts_unchanged:true,encoder_staging_default:"synchronized CPU copy",
    hardware_encoding_retained:true,physical_candidate_acceptance:false}' \
    > "$output/encoder-copy-overlay.json"
printf '%s\n' 'Retained video integration source, dependencies and lifecycle scripts verified.'
