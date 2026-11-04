#!/usr/bin/env bash
# COPR source preparation only. Retain the published development transaction.
set -Eeuo pipefail
export LC_ALL=C
test ! -e /etc/senemos/kernel-update.conf
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
output=$(realpath -m "${1:?COPR output}")
stage=$(mktemp -d)
trap 'find "$stage" -depth -delete' EXIT
mkdir -p "$output" "$stage/rpm" "$stage/inner"
curl -fLsS --retry 2 --proto '=https' --proto-redir '=https' \
 https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/srpm-builds/11260007/uke-boot-integration-1.8.60-1.fc46.src.rpm -o "$stage/parent.rpm"
printf '%s  %s\n' 8dd158e58f2e9d8bbd25da6d15072f55e79bda45c6b9caef3fe01d8b521a620c "$stage/parent.rpm" | sha256sum -c --strict
rpm2cpio "$stage/parent.rpm" > "$stage/input.cpio"
cpio -t --quiet < "$stage/input.cpio" > "$stage/entries"
awk '/^\/|(^|\/)\.\.($|\/)/ {bad=1} END {exit bad}' "$stage/entries"
(cd "$stage/rpm"; cpio -idm --quiet --no-absolute-filenames < "$stage/input.cpio")
tar -xJf "$stage/rpm/uke-boot-integration-1.8.60.tar.xz" -C "$stage"
outer=$stage/uke-boot-integration-1.8.60
tar -xJf "$outer/inputs/uke-boot-integration/uke-boot-integration-1.8.60.tar.xz" -C "$stage/inner"
native=$stage/inner/uke-boot-integration-1.8.60
for path in src/kernel-update.cpp src/userdata-update.hpp src/seamless-policy.hpp \
 src/profiles/kernel-update.conf tests/seamless-policy.cpp; do
 install -Dm644 "$owner/$path" "$native/$path"
done
[[ $(cat "$native/src/profiles/rollback.conf") == mode=enforced ]]
grep -Fxq '%config(noreplace) /etc/senemos/rollback.conf' "$stage/rpm/uke-boot-integration.spec"
sed 's/1[.]8[.]60/1.8.71/g' "$stage/rpm/uke-boot-integration.spec" > "$stage/version.spec"
awk '
 /^Provides: uke-kernel-update\(current-trial-replacement-v1\)/ {print "Provides: uke-kernel-update(seamless-policy-v2) = 1"}
 /    \/usr\/bin\/uke-kernel-update migrate-automatic-policy \|\| exit 1/ {
  print "    /usr/bin/uke-kernel-update enable-seamless-policy || exit 1"; migrated++
 }
 /^%check$/ {
  print
  print "mapfile -t seamless_sources < <(find %{_builddir} -type f -path '\''*/tests/seamless-policy.cpp'\'')"
  print "test ${#seamless_sources[@]} = 1"
  print "g++ -std=c++20 -O2 -UNDEBUG -Wall -Wextra -Werror -Wno-unused-function \"${seamless_sources[0]}\" -o %{_builddir}/seamless-policy-contract $(pkg-config --cflags --libs fdisk zlib openssl libsystemd)"
  print "%{_builddir}/seamless-policy-contract"
  next
 }
 {print}
 END {if(migrated!=1)exit 1}
' "$stage/version.spec" > "$output/uke-boot-integration.spec"
epoch=$(git -C "$owner" show -s --format=%ct HEAD)
mv "$native" "$stage/inner/uke-boot-integration-1.8.71"
rm "$outer/inputs/uke-boot-integration/uke-boot-integration-1.8.60.tar.xz"
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf \
 "$outer/inputs/uke-boot-integration/uke-boot-integration-1.8.71.tar.xz" -C "$stage/inner" uke-boot-integration-1.8.71
cp "$output/uke-boot-integration.spec" "$outer/inputs/uke-boot-integration/uke-boot-integration.spec"
jq -n --arg producer "$(git -C "$owner" rev-parse HEAD)" \
 '{schema_version:1,parent_build:11260007,producer_commit:$producer,seamless_default:true,
 rollback_default:"enforced",rollback_local_config_preserved:true,
 automatic_development_replacement_requires_local_off:true,physical_acceptance:false}' > "$outer/seamless-policy-v2-overlay.json"
mv "$outer" "$stage/uke-boot-integration-1.8.71"
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf \
 "$output/uke-boot-integration-1.8.71.tar.xz" -C "$stage" uke-boot-integration-1.8.71
[[ $(rpmspec -q --srpm --qf '%{VERSION}-%{RELEASE}' "$output/uke-boot-integration.spec") == 1.8.71-1.fc46 ]]
