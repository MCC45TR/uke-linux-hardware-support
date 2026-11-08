#!/usr/bin/bash
# COPR source preparation only; never compile or run an updater on the tablet.
set -Eeuo pipefail
test ! -e /etc/senemos/kernel-update.conf
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
output=$(realpath -m "${1:?COPR source result}")
[[ ! -e $output ]]
stage=$(mktemp -d)
trap 'find "$stage" -depth -delete' EXIT
curl -qfsSL --retry 2 --max-time 180 \
 https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/srpm-builds/11287194/uke-control-2.10.0-1.src.rpm -o "$stage/parent.rpm"
printf '%s  %s\n' ece44aeebaca724b13db354795557678969138dbac564544ec0549db944d8a9d "$stage/parent.rpm" | sha256sum -c --strict --quiet -
rpm2cpio "$stage/parent.rpm" > "$stage/parent.cpio"
cpio -t --quiet < "$stage/parent.cpio" | awk '/^\/|(^|\/)\.\.($|\/)/{bad=1}END{exit bad}'
mkdir "$output" "$stage/overlay"
(cd "$output";cpio -idm --quiet --no-absolute-filenames < "$stage/parent.cpio")
for file in boot-health-report.hpp;do install -m644 "$owner/../uke-boot/src/$file" "$stage/overlay/";done
for file in boot-health-report.cpp boot-health-report.sh;do install -m644 "$owner/../uke-boot/tests/$file" "$stage/overlay/";done
install -m644 "$owner/../uke-boot/patches/rollback-health-observation.patch" "$stage/overlay/"
install -m644 "$owner/packaging/apply-boot-health-report.sh" "$stage/overlay/"
install -m644 "$owner/docs/BOOT-HEALTH-OBSERVATION.md" "$stage/overlay/"
install -m644 "$owner/../LICENSE" "$stage/overlay/LICENSE"
tar --sort=name --mtime=@1794142800 --owner=0 --group=0 --numeric-owner -cJf "$output/uke-control-boot-health.tar.xz" -C "$stage/overlay" .
install -m644 "$owner/packaging/fold-health-spec.sh" "$owner/packaging/copr-control-health.sh" "$output/"
bash "$owner/packaging/fold-health-spec.sh" "$output/uke-control.spec"
sed -i 's/application_version="2.10.0"/application_version="2.11.0"/' "$output/control-version.hpp"
[[ $(rpmspec -q --srpm --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$output/uke-control.spec") == uke-control-2.11.0-1.fc46 ]]
rpmspec -q --qf '%{NAME}\n' "$output/uke-control.spec" | sort > "$stage/names"
printf '%s\n' uke-control uke-control-client-devel uke-control-tui > "$stage/expected"
cmp "$stage/names" "$stage/expected"
rpmspec -P "$output/uke-control.spec" > "$stage/expanded"
grep -Fx 'Requires: uke-control-tui(aarch-64) = 2.11.0-1.fc46' "$stage/expanded"
grep -Fx 'Provides: uke-control(kernel-health-report-v1) = 1' "$stage/expanded"
bash "$owner/tests/kernel-consolidation.sh" "$output/uke-control.spec"
printf 'Control health-report source prepared from admitted2.10; no extra runtime RPM, service, policy default or acceptance write.\n'
