#!/usr/bin/bash
# Source composition only. No new runtime family, service or tablet compiler.
set -Eeuo pipefail
test ! -e /etc/senemos/kernel-update.conf
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
output=$(realpath -m "${1:?COPR result directory}")
[[ ! -e $output ]]
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
curl -qfsSL --retry 2 --max-time 180 \
 https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/srpm-builds/11287797/uke-control-2.11.0-1.src.rpm -o "$work/parent.rpm"
printf '%s  %s\n' 345a5f7a5907b09ab1cfa3f013799864e4c0be7d98f991321c7301b7c8d0bd1a "$work/parent.rpm" | sha256sum -c --strict --quiet -
rpm2cpio "$work/parent.rpm" > "$work/parent.cpio"
cpio -t --quiet < "$work/parent.cpio" | awk '/^\/|(^|\/)\.\.($|\/)/{bad=1}END{exit bad}'
mkdir "$output" "$work/overlay"
(cd "$output";cpio -idm --quiet --no-absolute-filenames < "$work/parent.cpio")
for file in critical-health.hpp critical-runtime.hpp boot-health-report.hpp;do install -m644 "$owner/../uke-boot/src/$file" "$work/overlay/";done
for file in critical-health.cpp critical-health.sh critical-userdata.cpp boot-health-report.cpp;do install -m644 "$owner/../uke-boot/tests/$file" "$work/overlay/";done
install -m644 "$owner/../uke-boot/patches/critical-rollback.patch" "$owner/packaging/apply-critical-rollback.sh" "$owner/docs/CRITICAL-ROLLBACK.md" "$work/overlay/"
install -m644 "$owner/../LICENSE" "$work/overlay/LICENSE"
tar --sort=name --mtime=@1794168000 --owner=0 --group=0 --numeric-owner -cJf "$output/uke-control-critical-rollback.tar.xz" -C "$work/overlay" .
install -m644 "$owner/packaging/fold-critical-spec.sh" "$owner/packaging/copr-control-critical.sh" "$output/"
bash "$owner/packaging/fold-critical-spec.sh" "$output/uke-control.spec"
sed -i 's/application_version="2.11.0"/application_version="2.12.3"/' "$output/control-version.hpp"
[[ $(rpmspec -q --srpm --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$output/uke-control.spec") == uke-control-2.12.3-1.fc46 ]]
rpmspec -q --qf '%{NAME}\n' "$output/uke-control.spec" | sort > "$work/names"
printf '%s\n' uke-control uke-control-client-devel uke-control-tui > "$work/expected"
cmp "$work/names" "$work/expected"
printf 'Control critical source prepared from admitted2.11; existing updater/service/transaction and runtime families retained; native/device fallback remains separate.\n'
