#!/usr/bin/env bash
# COPR only: extend the entire qualified100 updater source capsule.
set -Eeuo pipefail
test ! -e /etc/senemos/kernel-update.conf
export LC_ALL=C
[[ $(rpm --eval '%{_host_cpu}') == aarch64 ]]
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
output=$(realpath -m "${1:?Source result directory}")
version=${2:?Unique updater version}
[[ $version == 1.8.101 ]]
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
[[ ! -e $output || (-d $output && -z $(find "$output" -mindepth 1 -maxdepth 1 -print -quit)) ]]
mkdir -p "$output" "$work/rpm" "$work/outer" "$work/inner"
curl -q -fLsS --retry 2 --max-time 180 https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/srpm-builds/11268277/uke-boot-integration-1.8.100-1.fc46.src.rpm -o "$work/parent.src.rpm"
printf '%s  %s\n' cef12922b9e63b90562fc974ac8d1791fdb336a1f1a4a01639ef92a6d070f21a "$work/parent.src.rpm" | sha256sum -c --strict -
rpm2cpio "$work/parent.src.rpm" > "$work/parent.cpio"
cpio -t --quiet < "$work/parent.cpio" | awk '/^\/|(^|\/)\.\.($|\/)/ {bad=1} END {exit bad}'
(cd "$work/rpm"; cpio -idm --quiet --no-absolute-filenames < "$work/parent.cpio")
tar -xJf "$work/rpm/uke-boot-integration-1.8.100.tar.xz" -C "$work/outer"
outer=$work/outer/uke-boot-integration-1.8.100
tar -xJf "$outer/inputs/uke-boot-integration/uke-boot-integration-1.8.100.tar.xz" -C "$work/inner"
native=$work/inner/uke-boot-integration-1.8.100
(cd "$native"; find . -type f ! -path ./src/userdata-update.hpp ! -path ./tests/slot-control.cpp -print0 | sort -z | xargs -0 sha256sum) > "$work/retained"
(cd "$native"; patch --dry-run -p1 --fuzz=0 --batch -i "$owner/patches/rollback-off-crc-maintenance.patch"; patch -p1 --fuzz=0 --batch -i "$owner/patches/rollback-off-crc-maintenance.patch")
(cd "$native"; sha256sum -c --strict --quiet "$work/retained")
[[ $(cat "$native/src/profiles/rollback.conf") == mode=enforced ]]
[[ $(cat "$native/src/profiles/kernel-update.conf" | sed '/^#/d;/^$/d') == automatic=1 ]]
[[ $(grep -Fx automatic=1 "$native/src/profiles/userdata-update.conf") == automatic=1 ]]
install -m644 "$owner/src/metadata-maintenance.hpp" "$native/src/metadata-maintenance.hpp"
install -m644 "$owner/docs/ROLLBACK-OFF-CRC.md" "$native/docs/ROLLBACK-OFF-CRC.md"
install -m644 "$owner/tests/rollback-off-crc.sh" "$native/tests/rollback-off-crc.sh"
spec=$output/uke-boot-integration.spec
awk '
 /^%changelog$/ {history=1;print;print "* Fri Nov 06 2026 Uke contributors - 1.8.101-1";
 print "- Repair matching-array backup CRC metadata independently of health acceptance.";
 print "- Preserve slot flags, type GUIDs, UFS selection and unapproved checkpoint state.";print "";next}
 !history {gsub(/1[.]8[.]100/,"1.8.101")}
 /^Provides: uke-kernel-update\(alternating-checkpoint-v2\)/ {capability++}
 /^bash tests\/alternating-prewrite[.]sh$/ {print;print "bash tests/rollback-off-crc.sh";next}
 /the global default is disabled/ {sub(/the global default is disabled/,"both package defaults are enabled")}
 {print} END {if(capability!=1)exit 1}
' "$work/rpm/uke-boot-integration.spec" > "$spec"
# The package layout and all original test/scriptlet sections remain intact.
[[ $(rpmspec -q --srpm --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$spec") == uke-boot-integration-1.8.101-1.fc46 ]]
install -m644 "$spec" "$outer/inputs/uke-boot-integration/uke-boot-integration.spec"
mv "$native" "$work/inner/uke-boot-integration-$version"
epoch=$(git -C "$owner" show -s --format=%ct HEAD)
unlink "$outer/inputs/uke-boot-integration/uke-boot-integration-1.8.100.tar.xz"
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf "$outer/inputs/uke-boot-integration/uke-boot-integration-$version.tar.xz" -C "$work/inner" "uke-boot-integration-$version"
git -C "$owner" rev-parse HEAD > "$outer/rollback-off-crc-source-commit"
printf 'parent_build=11268277\nparent_sha256=cef12922b9e63b90562fc974ac8d1791fdb336a1f1a4a01639ef92a6d070f21a\n' > "$outer/rollback-off-crc-parent"
cp "$outer/SOURCE-SHA256SUMS" "$outer/SOURCE-SHA256SUMS.before-crc-maintenance"
(cd "$outer"; find . -type f ! -name SOURCE-SHA256SUMS -print0 | sort -z | xargs -0 sha256sum > "$work/source-sha256")
install -m644 "$work/source-sha256" "$outer/SOURCE-SHA256SUMS"
mv "$outer" "$work/outer/uke-boot-integration-$version"
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf "$output/uke-boot-integration-$version.tar.xz" -C "$work/outer" "uke-boot-integration-$version"
printf 'Complete100 updater retained; CRC-only101 maintenance native/package and device qualification pending.\n'
