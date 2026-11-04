#!/usr/bin/env bash
# COPR only: keep the qualified native code and broaden the trigger to its directory.
set -Eeuo pipefail
export LC_ALL=C
test ! -e /etc/senemos/kernel-update.conf
output=$(realpath -m "${1:?COPR output}")
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
stage=$(mktemp -d)
trap 'find "$stage" -depth -delete' EXIT
mkdir -p "$output" "$stage/rpm" "$stage/inner"
curl -fLsS --retry 2 --proto '=https' --proto-redir '=https' \
 https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/srpm-builds/11260167/uke-boot-integration-1.8.71-1.fc46.src.rpm -o "$stage/parent.rpm"
printf '%s  %s\n' 48d7ab16adfd2b246ce9de26ce08aae9086bcc41ec593c71c01ecc40d4e04ee0 "$stage/parent.rpm" | sha256sum -c --strict
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
extract "$stage/rpm/uke-boot-integration-1.8.71.tar.xz" "$stage"
outer=$stage/uke-boot-integration-1.8.71
extract "$outer/inputs/uke-boot-integration/uke-boot-integration-1.8.71.tar.xz" "$stage/inner"
native=$stage/inner/uke-boot-integration-1.8.71
(cd "$native"; find src tests -type f -print0 | sort -z | xargs -0 sha256sum) > "$stage/native-before.sha256"
[[ $(cat "$native/src/profiles/rollback.conf") == mode=enforced ]]
grep -Fxq '%config(noreplace) /etc/senemos/rollback.conf' "$stage/rpm/uke-boot-integration.spec"
awk '
 /^%transfiletriggerin -n uke-kernel-update -- / {
    if($0!="%transfiletriggerin -n uke-kernel-update -- /usr/lib/senemos/kernel-updates /usr/lib/senemos/default-boot.img")exit 1
    print "%transfiletriggerin -n uke-kernel-update -- /usr/lib/senemos";count++;next
 }
 {gsub(/1[.]8[.]71/,"1.8.90");print}
 END {if(count!=1)exit 1}
' "$stage/rpm/uke-boot-integration.spec" > "$output/uke-boot-integration.spec"
[[ $(grep -Fc '%transfiletriggerin -n uke-kernel-update -- /usr/lib/senemos' "$output/uke-boot-integration.spec") == 1 ]]
mv "$native" "$stage/inner/uke-boot-integration-1.8.90"
(cd "$stage/inner/uke-boot-integration-1.8.90"; sha256sum -c --strict --quiet "$stage/native-before.sha256")
epoch=$(git -C "$owner" show -s --format=%ct HEAD)
rm "$outer/inputs/uke-boot-integration/uke-boot-integration-1.8.71.tar.xz"
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf \
 "$outer/inputs/uke-boot-integration/uke-boot-integration-1.8.90.tar.xz" -C "$stage/inner" uke-boot-integration-1.8.90
cp "$output/uke-boot-integration.spec" "$outer/inputs/uke-boot-integration/uke-boot-integration.spec"
jq -n --arg producer "$(git -C "$owner" rev-parse HEAD)" \
 '{schema_version:1,parent_build:11260167,producer_commit:$producer,trigger_prefix:"/usr/lib/senemos",native_code_and_tests_unchanged:true,default_rollback:"enforced",physical_acceptance:false}' \
 > "$outer/selected-profile-trigger-overlay.json"
mv "$outer" "$stage/uke-boot-integration-1.8.90"
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf \
 "$output/uke-boot-integration-1.8.90.tar.xz" -C "$stage" uke-boot-integration-1.8.90
[[ $(rpmspec -q --srpm --qf '%{VERSION}-%{RELEASE}' "$output/uke-boot-integration.spec") == 1.8.90-1.fc46 ]]
