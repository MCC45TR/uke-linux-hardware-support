#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# COPR source worker only; emit the retained metadata binary, not a backend.
set -Eeuo pipefail
export LC_ALL=C
test ! -e /sys/firmware/devicetree/base/model
[[ $(rpm --eval '%{_host_cpu}') == aarch64 && $(rpm --eval '%{fedora}') == 46 ]]
here=$(cd -- "$(dirname -- "$0")"; pwd)
output=$(realpath -m "${1:?COPR result directory}")
stage=$(mktemp -d)
trap 'rm -rf -- "$stage"' EXIT
curl -fLsS --max-time 40 \
    'https://copr.fedorainfracloud.org/api_3/build/list?ownername=mcc45tr&projectname=uke-linux-unstable&packagename=uke-control&limit=100' \
    -o "$stage/history.json"
jq -e '(.items|type)=="array" and (.items|length)<100 and all(.items[];
    ((.source_package.version // "")|sub("[.]fc[0-9]+$";""))!="2.8.1-5.3")' \
    "$stage/history.json" >/dev/null
curl -fLsS --proto '=https' --proto-redir '=https' --max-time 90 \
    https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/fedora-rawhide-aarch64/Packages/u/uke-control-2.8.1-5.2.fc46.src.rpm \
    -o "$stage/parent.src.rpm"
printf '%s  %s\n' 4b738430b96bb90bbaa76cc1e8b91867e77c588b15a2521de3d736ad5a18bbdf "$stage/parent.src.rpm" | sha256sum -c -
curl -fLsS --proto '=https' --proto-redir '=https' --max-time 30 \
    https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/pubkey.gpg \
    -o "$stage/key.asc"
mkdir -m700 "$stage/gpg" "$stage/keys"
fingerprint=$(gpg --homedir "$stage/gpg" --batch --show-keys --with-colons "$stage/key.asc" 2>/dev/null | awk -F: '$1=="fpr"{print $10}')
[[ $fingerprint == 2204A661EA630A5303500085538797E72A3E4ECA ]]
rpmkeys --dbpath "$stage/keys" --import "$stage/key.asc"
rpmkeys --dbpath "$stage/keys" --checksig "$stage/parent.src.rpm"
[[ $(rpm -qp --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$stage/parent.src.rpm") == uke-control-2.8.1-5.2.fc46 ]]
rpm2cpio "$stage/parent.src.rpm" > "$stage/input.cpio"
cpio -t --quiet < "$stage/input.cpio" > "$stage/members"
awk '/^\/|(^|\/)\.\.($|\/)/ {bad=1} END{exit bad}' "$stage/members"
[[ $(wc -l < "$stage/members") == 4 ]]
mkdir "$stage/input" "$output"
(cd "$stage/input"; cpio -idm --quiet --no-absolute-filenames < "$stage/input.cpio")
cp "$stage/input/"* "$output/"
awk -f "$here/retained-graphics.awk" "$stage/input/uke-control.spec" > "$output/uke-control.spec"
cmp "$stage/input/uke-control-2.8.1.tar.xz" "$output/uke-control-2.8.1.tar.xz"
sections() { awk '/^%changelog/{p=0} /^%(pre|post|preun|postun|pretrans|posttrans|files)([[:space:]]|$)/{p=($0 ~ /-n uke-core-meta$/)} p{print}' "$1"; }
sections "$stage/input/uke-control.spec" > "$stage/sections.before"
sections "$output/uke-control.spec" > "$stage/sections.after"
cmp "$stage/sections.before" "$stage/sections.after"
[[ $(grep -c '^%package ' "$output/uke-control.spec") == 1 ]]
[[ $(grep -c '^%files ' "$output/uke-control.spec") == 1 ]]
rpmspec --target aarch64 -q --qf '%{NAME}\t%{VERSION}-%{RELEASE}\n' "$output/uke-control.spec" > "$stage/identities"
cat "$stage/identities"
grep -Fxq $'uke-control\t2.8.1-5.3.fc46' "$stage/identities"
grep -Fxq $'uke-core-meta\t1.0.7-19.2.fc46' "$stage/identities"
[[ $(wc -l < "$stage/identities") == 2 ]]
commit=$(git -C "$here" rev-parse HEAD)
jq -n --arg commit "$commit" '{source_commit:$commit,parent_copr_build:11247345,
    signed_parent_sha256:"4b738430b96bb90bbaa76cc1e8b91867e77c588b15a2521de3d736ad5a18bbdf",
    original_archive_and_core_payload_unchanged:true,lifecycle_scripts_unchanged:true,
    backend_sdk_and_power_packages_produced:false,
    added_requirements:["uke-mesa-libOpenCL","uke-mesa-libclc22-spirv","uke-vaapi-v4l2 >= 1.0.0-12.fc46"],
    va_backend_copr_build:11251033,device_test:"72 independently decoded H264 frames at three sizes; exact 30-frame decode regression"}' \
    > "$output/core-graphics-overlay.json"
printf '%s\n' 'Retained Core-only metadata source, payload and lifecycle sections verified.'
