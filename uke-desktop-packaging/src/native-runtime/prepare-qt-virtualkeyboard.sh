#!/bin/bash
# COPR-only source preparation; no upstream program is executed here.
set -Eeuo pipefail
export LC_ALL=C
test ! -e /etc/senemos/kernel-update.conf
[[ $(rpm --eval '%{_host_cpu}') == aarch64 && $(rpm --eval '%{fedora}') == 46 ]]
project=$(cd -- "$(dirname -- "$0")/../.."; pwd)
manifest=$project/manifests/qt-virtualkeyboard-sound.json
output=$(realpath -m "${1:?COPR result directory}")
stage=$(mktemp -d)
keydb=$(mktemp -d /tmp/uke-qt-keydb.XXXXXX)
trap 'find "$stage" "$keydb" -depth -delete' EXIT
for channel in uke-linux-unstable uke-linux-test uke-linux; do
    curl -fsS --max-time 30 --get --data-urlencode ownername=mcc45tr --data-urlencode "projectname=$channel" --data-urlencode packagename=qt6-qtvirtualkeyboard --data-urlencode limit=100 https://copr.fedorainfracloud.org/api_3/build/list > "$stage/builds.json"
    jq -e '(.items|type)=="array" and (.items|length)<100 and all(.items[]; ((.source_package.version // "")|sub("[.]fc[0-9]+$";""))!="6.12.0-1.1")' "$stage/builds.json" >/dev/null
done
for kind in source_rpm signed_source; do
    curl -fsS --proto '=https' --proto-redir '=https' --max-time 120 "$(jq -er ".${kind}_url" "$manifest")" -o "$stage/$kind.rpm"
    printf '%s  %s\n' "$(jq -er ".${kind}_sha256" "$manifest")" "$stage/$kind.rpm" | sha256sum -c -
    [[ $(rpm -qp --qf '%{NAME}-%{VERSION}-%{RELEASE}:%{SOURCEPACKAGE}' "$stage/$kind.rpm") == qt6-qtvirtualkeyboard-6.12.0-1.fc46:1 ]]
    rpm2cpio "$stage/$kind.rpm" > "$stage/$kind.cpio"
done
cmp "$stage/source_rpm.cpio" "$stage/signed_source.cpio"
printf '%s  %s\n' "$(jq -er .source_payload_sha256 "$manifest")" "$stage/source_rpm.cpio" | sha256sum -c -
# A separate top-level DB avoids inherited home/scratch SELinux traversal limits.
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
top=$stage/rpmbuild
mkdir -p "$top"/{BUILD,BUILDROOT,RPMS,SRPMS,SOURCES,SPECS} "$output"
cpio -t --quiet < "$stage/source_rpm.cpio" | awk '/^\/|(^|\/)\.\.($|\/)/{bad=1}END{exit bad}'
(cd "$top/SOURCES"; cpio -idm --quiet --no-absolute-filenames < "$stage/source_rpm.cpio")
printf '%s  %s\n' "$(jq -er .upstream_tar_sha256 "$manifest")" "$top/SOURCES/qtvirtualkeyboard-everywhere-src-6.12.0.tar.xz" | sha256sum -c -
bash "$project/tests/qt-virtualkeyboard-source.sh" "$top/SOURCES/qt6-qtvirtualkeyboard.spec"
awk -v release="$(jq -er .release "$manifest")" -f "$project/src/native-runtime/qt-virtualkeyboard-sound.awk" "$top/SOURCES/qt6-qtvirtualkeyboard.spec" > "$top/SPECS/qt6-qtvirtualkeyboard.spec"
cp "$stage/source_rpm.rpm" "$top/SOURCES/qt6-qtvirtualkeyboard-6.12.0-1.fc46.src.rpm"
cp "$project/tests/qt-virtualkeyboard-sound.cpp" "$top/SOURCES/"
jq --arg commit "$(git -C "$project" rev-parse HEAD)" '. + {recipe_commit:$commit}' "$manifest" > "$top/SOURCES/qt-virtualkeyboard-build-source.json"
[[ $(rpmspec --define 'dist .fc46' -q --srpm --qf '%{VERSION}-%{RELEASE}' "$top/SPECS/qt6-qtvirtualkeyboard.spec") == 6.12.0-1.1.fc46 ]]
rpmbuild -bs --nodeps --target aarch64 --define 'dist .fc46' --define "_topdir $top" "$top/SPECS/qt6-qtvirtualkeyboard.spec"
install -m644 "$top/SRPMS/qt6-qtvirtualkeyboard-6.12.0-1.1.fc46.src.rpm" "$output/"
