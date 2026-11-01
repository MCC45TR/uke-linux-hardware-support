#!/bin/bash
# COPR-only source preparation. Never runs upstream scripts or target programs.
set -Eeuo pipefail
export LC_ALL=C
test ! -e /etc/senemos/kernel-update.conf
[[ $(rpm --eval '%{_host_cpu}') == aarch64 && $(rpm --eval '%{fedora}') == 46 ]]
project=$(cd -- "$(dirname -- "$0")/../.."; pwd)
manifest=$project/manifests/tailscale-dns-label.json
output=$(realpath -m "${1:?COPR result directory}")
stage=$(mktemp -d)
keydb=$(mktemp -d /tmp/uke-tailscale-keydb.XXXXXX)
trap 'find "$stage" "$keydb" -depth -delete' EXIT
for channel in uke-linux-unstable uke-linux-test uke-linux; do
 curl -fsS --max-time 30 --get --data-urlencode ownername=mcc45tr --data-urlencode "projectname=$channel" --data-urlencode packagename=tailscale --data-urlencode limit=100 https://copr.fedorainfracloud.org/api_3/build/list > "$stage/builds.json"
 jq -e '(.items|type)=="array" and (.items|length)<100 and all(.items[]; ((.source_package.version//"")|sub("[.]fc[0-9]+$";""))!="1.104.4-1.1")' "$stage/builds.json" >/dev/null
done
for kind in source_rpm signed_source; do
 curl -fLsS --proto '=https' --proto-redir '=https' --max-time 180 "$(jq -er ".${kind}_url" "$manifest")" -o "$stage/$kind.rpm"
 printf '%s  %s\n' "$(jq -er ".${kind}_sha256" "$manifest")" "$stage/$kind.rpm" | sha256sum -c -
 [[ $(rpm -qp --qf '%{NAME}-%{VERSION}-%{RELEASE}:%{SOURCEPACKAGE}' "$stage/$kind.rpm") == tailscale-1.104.4-1.fc46:1 ]]
 rpm2cpio "$stage/$kind.rpm" > "$stage/$kind.cpio"
done
cmp "$stage/source_rpm.cpio" "$stage/signed_source.cpio"
printf '%s  %s\n' "$(jq -er .source_payload_sha256 "$manifest")" "$stage/source_rpm.cpio" | sha256sum -c -
if [[ -r /sys/fs/selinux/enforce && $(cat /sys/fs/selinux/enforce) == 1 ]]; then
 chcon --reference="$(rpm --eval '%{_dbpath}')" "$keydb"
fi
rpm --dbpath "$keydb" --initdb
rpmkeys --dbpath "$keydb" --import "$project/../packaging/families/keys/fedora-46.asc"
rpmkeys --dbpath "$keydb" --list | awk '{print toupper($1)}' > "$stage/keys.txt"
printf '%s\n' "$(jq -er .fedora_key_fingerprint "$manifest")" | cmp - "$stage/keys.txt"
rpmkeys --dbpath "$keydb" --checksig --verbose "$stage/signed_source.rpm" > "$stage/signature.txt"
grep -Fq 'signature, key fingerprint: d924b10d3e810dabdd8b56b596e7e91491211fce: OK' "$stage/signature.txt"
! grep -Eq 'NOKEY|NOT OK|BAD|NOTTRUSTED' "$stage/signature.txt"
mkdir -p "$stage/sources" "$output"
cpio -t --quiet < "$stage/source_rpm.cpio" | awk '/^\/|(^|\/)\.\.($|\/)/{bad=1}END{exit bad}'
(cd "$stage/sources"; cpio -idm --quiet --no-absolute-filenames < "$stage/source_rpm.cpio")
printf '%s  %s\n' "$(jq -er .vendor_archive_sha256 "$manifest")" "$stage/sources/tailscale-1.104.4-vendored.tar.xz" | sha256sum -c -
bash "$project/tests/tailscale-source.sh" "$stage/sources/tailscale.spec"
awk -v release="$(jq -er .release "$manifest")" -f "$project/src/native-runtime/tailscale-dns-label.awk" "$stage/sources/tailscale.spec" > "$output/tailscale.spec"
for file in tailscale-1.104.4-vendored.tar.xz tailscaled.service tailscale-systray-user.service create-vendor-tarball.sh tailscale-1.92.5-systray-unit.patch; do
 install -m644 "$stage/sources/$file" "$output/$file"
done
install -m644 "$stage/source_rpm.rpm" "$output/tailscale-1.104.4-1.fc46.src.rpm"
install -m644 "$project/patches/tailscale/direct-prelabel.patch" "$output/tailscale-direct-prelabel.patch"
install -m644 "$project/tests/tailscale-resolver-label_test.go" "$output/"
jq --arg commit "$(git -C "$project" rev-parse HEAD)" '. + {recipe_commit:$commit}' "$manifest" > "$output/tailscale-dns-build-source.json"
[[ $(rpmspec --define 'dist .fc46' -q --srpm --qf '%{VERSION}-%{RELEASE}' "$output/tailscale.spec") == 1.104.4-1.1.fc46 ]]
