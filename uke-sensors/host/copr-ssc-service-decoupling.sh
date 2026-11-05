#!/usr/bin/env bash
# COPR only. Extend the complete installed sensor source family, not stale git.
set -Eeuo pipefail
test ! -e /etc/senemos/kernel-update.conf
export LC_ALL=C
[[ $(rpm --eval '%{_host_cpu}') == aarch64 && $(rpm --eval '%{fedora}') == 46 ]]
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
output=$(realpath -m "${1:?Source result directory}")
[[ ${2:?Unique source release} == 5 && ${3:?Unique fileservice release} == 31 ]]
[[ ! -e $output || (-d $output && -z $(find "$output" -mindepth 1 -maxdepth 1 -print -quit)) ]]
work=$(mktemp -d);trap 'find "$work" -depth -delete' EXIT
mkdir -p "$output" "$work/rpm" "$work/source"
curl -q -fLsS --retry 2 --max-time 90 https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/srpm-builds/11239521/uke-sensor-integration-1.0.0-3.src.rpm -o "$work/parent.src.rpm"
printf '%s  %s\n' 9cdc893e2b96ce1ec3e7b992a4ea4d2d3f11178c38d827a56b0af398104ffe5a "$work/parent.src.rpm" | sha256sum -c --strict -
rpm2cpio "$work/parent.src.rpm" > "$work/source.cpio"
cpio -t --quiet < "$work/source.cpio" | awk '/^\/|(^|\/)\.\.($|\/)/ {bad=1} END {exit bad}'
(cd "$work/rpm";cpio -idm --quiet --no-absolute-filenames < "$work/source.cpio")
archive=uke-sensor-integration-1.0.0.tar.xz
tar -xJf "$work/rpm/$archive" -C "$work/source"
source=$work/source/uke-sensor-integration-1.0.0
(cd "$source";sha256sum -c --strict --quiet SOURCE-SHA256SUMS)
# All original native sources, firmware guards and tests remain byte-identical.
cp "$source/SOURCE-SHA256SUMS" "$source/SOURCE-SHA256SUMS.before-service-decoupling"
install -m644 "$owner/docs/SSC-SERVICE-DECOUPLING.md" "$source/SSC-SERVICE-DECOUPLING.md"
install -m644 "$owner/manifests/ssc-service-decoupling-20261106.json" "$source/service-decoupling.json"
git -C "$owner" rev-parse HEAD > "$source/service-decoupling-source-commit"
(cd "$source";sha256sum -c --strict --quiet SOURCE-SHA256SUMS.before-service-decoupling)
(cd "$source";find . -type f ! -name SOURCE-SHA256SUMS -print0 | sort -z | xargs -0 sha256sum > "$work/seal")
install -m644 "$work/seal" "$source/SOURCE-SHA256SUMS"
epoch=$(git -C "$owner" show -s --format=%ct HEAD)
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf "$output/$archive" -C "$work/source" uke-sensor-integration-1.0.0
awk '
 /^%(package|description|prep|build|install|check|post|preun|postun|transfiletriggerin|files|changelog)([[:space:]]|$)/ {
  skip=($0 ~ /-n uke-(front-color|proximity-capture)([[:space:]]|$)/)
 }
 skip {next}
 /^Release: 3%\{[?]dist\}$/ {$0="Release: 5%{?dist}";source_release++}
 /^Release: 29[.]fc46$/ {$0="Release: 31.fc46";binary_release++}
 /^Requires: iio-sensor-proxy\((ssc|uke-dual-light)\) >= 1$/ {removed++;next}
 /^%check$/ {
  print "# Do not republish completed observation clients with a service-only update."
  print "rm -f %{buildroot}/usr/libexec/uke-front-color %{buildroot}/usr/lib/systemd/system/uke-front-color.service %{buildroot}/usr/lib/systemd/system-preset/90-uke-front-color.preset %{buildroot}/usr/bin/uke-proximity-capture"
  install_cleanup++
 }
 /^%changelog$/ {
  print;print "* Fri Nov 06 2026 Uke contributors - 1.0.0-5";
  print "- Deliver fileservice release31 without unused SSC/dual-light frontend requirements.";
  print "- Preserve native sources, read-only factory admission and upgrade behavior.";
  print "- Publish only the required service and its matching debug packages.";print "";next
 }
 {print}
 END {if(source_release!=1 || binary_release!=3 || removed!=2 || install_cleanup!=1)exit 1}
' "$work/rpm/uke-sensor-integration.spec" > "$output/uke-sensor-integration.spec"
spec=$output/uke-sensor-integration.spec
[[ $(rpmspec -q --srpm --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$spec") == uke-sensor-integration-1.0.0-5.fc46 ]]
rpmspec -q --qf '%{NAME}-%{VERSION}-%{RELEASE}\n' "$spec" | sort > "$work/packages"
# rpmspec also lists the source-only header; no main %files means no RPM.
[[ $(grep -Ec '^%files' "$spec") == 3 && $(grep -Ec '^%files -n uke-ssc-fileservice([[:space:]]|-)' "$spec") == 3 ]]
printf '%s\n' uke-sensor-integration-1.0.0-5.fc46 uke-ssc-fileservice-2026.09.21-31.fc46 uke-ssc-fileservice-debuginfo-2026.09.21-31.fc46 uke-ssc-fileservice-debugsource-2026.09.21-31.fc46 | sort > "$work/expected"
cmp "$work/expected" "$work/packages"
rpmspec -q --requires "$spec" > "$work/requires"
! grep -F 'iio-sensor-proxy(' "$work/requires"
printf 'Complete sensor family retained; only fileservice31 and matched debug outputs. Native/package qualification pending.\n'
