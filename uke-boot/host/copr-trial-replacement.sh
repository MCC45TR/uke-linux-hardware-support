#!/usr/bin/env bash
# COPR source preparation only. Retain the exact installed native baseline.
set -Eeuo pipefail
export LC_ALL=C
test ! -e /etc/senemos/kernel-update.conf
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
output=$(realpath -m "${1:?COPR output}")
stage=$(mktemp -d)
trap 'find "$stage" -depth -delete' EXIT
mkdir -p "$output" "$stage/rpm" "$stage/inner"
curl -fLsS --retry 2 --proto '=https' --proto-redir '=https' \
 https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/srpm-builds/11259682/uke-boot-integration-1.8.53-1.fc46.src.rpm -o "$stage/parent.rpm"
printf '%s  %s\n' c1d18f5f33c4424de7d9d5b91327370a49f18af573ecfdd6dbdb92d1e999f28b "$stage/parent.rpm" | sha256sum -c --strict
rpmkeys --checksig --nosignature --verbose "$stage/parent.rpm"
rpm2cpio "$stage/parent.rpm" > "$stage/input.cpio"
cpio -t --quiet < "$stage/input.cpio" > "$stage/entries"
awk '/^\/|(^|\/)\.\.($|\/)/ {bad=1} END {exit bad}' "$stage/entries"
(cd "$stage/rpm"; cpio -idm --quiet --no-absolute-filenames < "$stage/input.cpio")
extract() {
    tar -tJf "$1" > "$stage/entries"
    awk '/^\/|(^|\/)\.\.($|\/)/ {bad=1} END {exit bad}' "$stage/entries"
    tar -xJf "$1" -C "$2"
}
extract "$stage/rpm/uke-boot-integration-1.8.53.tar.xz" "$stage"
outer=$stage/uke-boot-integration-1.8.53
extract "$outer/inputs/uke-boot-integration/uke-boot-integration-1.8.53.tar.xz" "$stage/inner"
native=$stage/inner/uke-boot-integration-1.8.53
(
    cd "$native"
    sha256sum -c --strict <<'BASELINE'
ad2d0538b8d9e01e004137fd340cce0dd61cd9aa2d31f9d3e9d92140b5271626  src/kernel-update.cpp
ae0ba2b13957a67fa004fb9a03dfff58fa45c070992f9ee3e5c08fc60ea187a8  src/userdata-update.hpp
7bee84aaea286a0f345f4864226781eb190b72f8aca99fa2f220db74859ce800  src/slot-control.hpp
4b2825972f5c4e16b2f8db483e5b6b16d78a9f08704f70c2f69745f0022818dd  tests/slot-control.cpp
BASELINE
)
for path in src/kernel-update.cpp src/userdata-update.hpp src/slot-control.hpp src/development-trial.hpp \
 tests/slot-control.cpp tests/userdata-update.sh tests/development-trial.cpp tests/development-trial.sh; do
    install -Dm644 "$owner/$path" "$native/$path"
done
sed 's/1[.]8[.]53/1.8.60/g' "$stage/rpm/uke-boot-integration.spec" > "$stage/version.spec"
awk '
 /^Provides: uke-kernel-update\(rollback-policy-v1\)/ {print "Provides: uke-kernel-update(current-trial-replacement-v1) = 1"}
 /^bash tests\/userdata-update.sh$/ {print;print "bash tests/development-trial.sh";next}
 {print}
' "$stage/version.spec" > "$output/uke-boot-integration.spec"
[[ $(grep -Fc 'Provides: uke-kernel-update(current-trial-replacement-v1) = 1' "$output/uke-boot-integration.spec") == 1 ]]
[[ $(grep -Fc 'bash tests/development-trial.sh' "$output/uke-boot-integration.spec") == 1 ]]
epoch=$(git -C "$owner" show -s --format=%ct HEAD)
mv "$native" "$stage/inner/uke-boot-integration-1.8.60"
rm "$outer/inputs/uke-boot-integration/uke-boot-integration-1.8.53.tar.xz"
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf \
 "$outer/inputs/uke-boot-integration/uke-boot-integration-1.8.60.tar.xz" -C "$stage/inner" uke-boot-integration-1.8.60
cp "$output/uke-boot-integration.spec" "$outer/inputs/uke-boot-integration/uke-boot-integration.spec"
jq -n --arg producer "$(git -C "$owner" rev-parse HEAD)" \
 '{schema_version:1,parent_build:11259682,producer_commit:$producer,default_rollback:"enforced",explicit_current_a_trial_only:true,retains_b_image:true,physical_acceptance:false}' \
 > "$outer/development-trial-overlay.json"
mv "$outer" "$stage/uke-boot-integration-1.8.60"
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf \
 "$output/uke-boot-integration-1.8.60.tar.xz" -C "$stage" uke-boot-integration-1.8.60
[[ $(rpmspec -q --srpm --qf '%{VERSION}-%{RELEASE}' "$output/uke-boot-integration.spec") == 1.8.60-1.fc46 ]]
