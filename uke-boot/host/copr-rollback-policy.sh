#!/usr/bin/env bash
# COPR source preparation only; preserve the published touch/RTC composition.
set -Eeuo pipefail
export LC_ALL=C
test ! -e /etc/senemos/kernel-update.conf
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
output=$(realpath -m "${1:?COPR output}")
stage=$(mktemp -d)
trap 'find "$stage" -depth -delete' EXIT
mkdir -p "$output" "$stage/rpm" "$stage/inner"
curl -fLsS --retry 2 --proto '=https' --proto-redir '=https' \
 https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/srpm-builds/11259613/uke-boot-integration-1.8.40-1.fc46.src.rpm -o "$stage/parent.rpm"
printf '%s  %s\n' 372b29921992cbbe69e424c3e5ad0cb0e20486867912751efeb4d15b97ecd5d1 "$stage/parent.rpm" | sha256sum -c --strict
rpm2cpio "$stage/parent.rpm" > "$stage/input.cpio"
cpio -t --quiet < "$stage/input.cpio" > "$stage/entries"
awk '/^\/|(^|\/)\.\.($|\/)/ {bad=1} END {exit bad}' "$stage/entries"
(cd "$stage/rpm"; cpio -idm --quiet --no-absolute-filenames < "$stage/input.cpio")
tar -xJf "$stage/rpm/uke-boot-integration-1.8.40.tar.xz" -C "$stage"
outer=$stage/uke-boot-integration-1.8.40
tar -xJf "$outer/inputs/uke-boot-integration/uke-boot-integration-1.8.40.tar.xz" -C "$stage/inner"
native=$stage/inner/uke-boot-integration-1.8.40
for path in src/kernel-update.cpp src/userdata-update.hpp src/installation-policy.hpp src/rollback-policy.hpp \
 tests/rollback-policy.cpp src/profiles/rollback.conf; do
 install -Dm644 "$owner/$path" "$native/$path"
done
sed 's/1[.]8[.]40/1.8.50/g' "$stage/rpm/uke-boot-integration.spec" > "$output/uke-boot-integration.spec"
awk '
 /^Provides: uke-kernel-update\(slot-selection-v1\)/ {print "Provides: uke-kernel-update(rollback-policy-v1) = 1"}
 /^install -Dm644 src\/profiles\/kernel-update.conf / {
  print "install -Dm644 src/profiles/rollback.conf %{buildroot}/etc/senemos/rollback.conf"
 }
 /^%config\(noreplace\) \/etc\/senemos\/kernel-update.conf$/ {print "%config(noreplace) /etc/senemos/rollback.conf"}
 /^%check$/ {
  print; print "mapfile -t policy_sources < <(find %{_builddir} -type f -path '\''*/tests/rollback-policy.cpp'\'')"
  print "test ${#policy_sources[@]} = 1"
  print "g++ -std=c++20 -O2 -UNDEBUG -Wall -Wextra -Werror -Wno-unused-function \"${policy_sources[0]}\" -o %{_builddir}/rollback-policy-contract $(pkg-config --cflags --libs fdisk zlib openssl libsystemd)"
  print "%{_builddir}/rollback-policy-contract"; next
 }
 {print}
' "$output/uke-boot-integration.spec" > "$stage/next.spec"
mv "$stage/next.spec" "$output/uke-boot-integration.spec"
epoch=$(git -C "$owner" show -s --format=%ct HEAD)
mv "$native" "$stage/inner/uke-boot-integration-1.8.50"
rm "$outer/inputs/uke-boot-integration/uke-boot-integration-1.8.40.tar.xz"
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf \
 "$outer/inputs/uke-boot-integration/uke-boot-integration-1.8.50.tar.xz" -C "$stage/inner" uke-boot-integration-1.8.50
cp "$output/uke-boot-integration.spec" "$outer/inputs/uke-boot-integration/uke-boot-integration.spec"
jq -n --arg producer "$(git -C "$owner" rev-parse HEAD)" \
 '{schema_version:1,parent_build:11259613,producer_commit:$producer,default_rollback:"enforced",physical_acceptance:false}' > "$outer/rollback-policy-overlay.json"
mv "$outer" "$stage/uke-boot-integration-1.8.50"
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf \
 "$output/uke-boot-integration-1.8.50.tar.xz" -C "$stage" uke-boot-integration-1.8.50
[[ $(rpmspec -q --srpm --qf '%{VERSION}-%{RELEASE}' "$output/uke-boot-integration.spec") == 1.8.50-1.fc46 ]]
