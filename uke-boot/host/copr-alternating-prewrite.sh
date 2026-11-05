#!/usr/bin/env bash
# COPR only: extend the entire qualified98 updater source capsule.
set -Eeuo pipefail
test ! -e /etc/senemos/kernel-update.conf
export LC_ALL=C
[[ $(rpm --eval '%{_host_cpu}') == aarch64 ]]
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
output=$(realpath -m "${1:?Source result directory}")
version=${2:?Unique updater version}
[[ $version == 1.8.99 ]]
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
[[ ! -e $output || (-d $output && -z $(find "$output" -mindepth 1 -maxdepth 1 -print -quit)) ]]
mkdir -p "$output" "$work/rpm" "$work/outer" "$work/inner"
curl -q -fLsS --retry 2 --max-time 180 https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/srpm-builds/11268046/uke-boot-integration-1.8.98-1.fc46.src.rpm -o "$work/parent.src.rpm"
printf '%s  %s\n' 0e09dab36a189ee135a4472233f64d15d7dbfb32ce4e449526d0b149e205845d "$work/parent.src.rpm" | sha256sum -c --strict -
rpm2cpio "$work/parent.src.rpm" > "$work/parent.cpio"
cpio -t --quiet < "$work/parent.cpio" | awk '/^\/|(^|\/)\.\.($|\/)/ {bad=1} END {exit bad}'
(cd "$work/rpm"; cpio -idm --quiet --no-absolute-filenames < "$work/parent.cpio")
tar -xJf "$work/rpm/uke-boot-integration-1.8.98.tar.xz" -C "$work/outer"
outer=$work/outer/uke-boot-integration-1.8.98
tar -xJf "$outer/inputs/uke-boot-integration/uke-boot-integration-1.8.98.tar.xz" -C "$work/inner"
native=$work/inner/uke-boot-integration-1.8.98
(cd "$native"; find . -type f ! -path ./src/kernel-update.cpp ! -path ./src/userdata-update.hpp ! -path ./src/slot-control.hpp ! -path ./src/seamless-policy.hpp ! -path ./tests/userdata-update.cpp ! -path ./tests/slot-control.cpp ! -path ./tests/seamless-policy.cpp -print0 | sort -z | xargs -0 sha256sum) > "$work/retained"
(cd "$native"; patch --dry-run -p1 --fuzz=0 --batch -i "$owner/patches/alternating-prewrite.patch"; patch -p1 --fuzz=0 --batch -i "$owner/patches/alternating-prewrite.patch")
(cd "$native"; sha256sum -c --strict --quiet "$work/retained")
[[ $(cat "$native/src/profiles/rollback.conf") == mode=enforced ]]
[[ $(cat "$native/src/profiles/kernel-update.conf" | sed '/^#/d;/^$/d') == automatic=1 ]]
[[ $(grep -Fx automatic=1 "$native/src/profiles/userdata-update.conf") == automatic=1 ]]
install -m644 "$owner/docs/ALTERNATING-CHECKPOINTS.md" "$native/docs/ALTERNATING-CHECKPOINTS.md"
install -m644 "$owner/tests/alternating-prewrite.sh" "$native/tests/alternating-prewrite.sh"
spec=$output/uke-boot-integration.spec
awk '
 /^%changelog$/ {history=1;print;print "* Fri Nov 06 2026 Uke contributors - 1.8.99-1";
 print "- Alternate inactive slots independently of rollback-off health policy.";
 print "- Preserve required accepted-kernel components and explicit unapproved checkpoints.";print "";next}
 !history {gsub(/1[.]8[.]98/,"1.8.99")}
 /^Provides: uke-kernel-update\(alternating-checkpoint-v2\)/ {capability++}
 /^bash tests\/slot-control[.]sh$/ {print;print "bash tests/alternating-prewrite.sh";next}
 /the global default is disabled/ {sub(/the global default is disabled/,"both package defaults are enabled")}
 {print} END {if(capability!=1)exit 1}
' "$work/rpm/uke-boot-integration.spec" > "$spec"
# The package layout and all original test/scriptlet sections remain intact.
[[ $(rpmspec -q --srpm --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$spec") == uke-boot-integration-1.8.99-1.fc46 ]]
install -m644 "$spec" "$outer/inputs/uke-boot-integration/uke-boot-integration.spec"
mv "$native" "$work/inner/uke-boot-integration-$version"
epoch=$(git -C "$owner" show -s --format=%ct HEAD)
unlink "$outer/inputs/uke-boot-integration/uke-boot-integration-1.8.98.tar.xz"
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf "$outer/inputs/uke-boot-integration/uke-boot-integration-$version.tar.xz" -C "$work/inner" "uke-boot-integration-$version"
git -C "$owner" rev-parse HEAD > "$outer/alternating-prewrite-source-commit"
printf 'parent_build=11268046\nparent_sha256=0e09dab36a189ee135a4472233f64d15d7dbfb32ce4e449526d0b149e205845d\n' > "$outer/alternating-prewrite-parent"
cp "$outer/SOURCE-SHA256SUMS" "$outer/SOURCE-SHA256SUMS.before-prewrite"
(cd "$outer"; find . -type f ! -name SOURCE-SHA256SUMS -print0 | sort -z | xargs -0 sha256sum > "$work/source-sha256")
install -m644 "$work/source-sha256" "$outer/SOURCE-SHA256SUMS"
mv "$outer" "$work/outer/uke-boot-integration-$version"
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf "$output/uke-boot-integration-$version.tar.xz" -C "$work/outer" "uke-boot-integration-$version"
printf 'Complete98 updater retained; actual checkpoint pre-write99 fixtures and device qualification pending.\n'
