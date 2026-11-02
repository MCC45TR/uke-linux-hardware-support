#!/bin/bash
# COPR-only: retain the qualified family and poll native startup readiness briefly.
set -Eeuo pipefail
export LC_ALL=C
test ! -e /etc/senemos/kernel-update.conf
[[ $(rpm --eval '%{_host_cpu}') == aarch64 && $(rpm --eval '%{fedora}') == 46 ]]
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
output=$(realpath -m "${1:?COPR result directory}")
stage=$(mktemp -d)
trap 'find "$stage" -depth -delete' EXIT
for project in uke-linux-unstable uke-linux-test uke-linux; do
 curl --fail --silent --show-error --max-time 45 --get --data-urlencode ownername=mcc45tr \
  --data-urlencode "projectname=$project" --data-urlencode packagename=uke-boot-integration \
  --data-urlencode limit=100 https://copr.fedorainfracloud.org/api_3/build/list > "$stage/builds.json"
 jq -e '(.items|type)=="array" and (.items|length)<100 and
  all(.items[]; ((.source_package.version // "")|sub("[.]fc[0-9]+([.][0-9]+)*$";""))!="1.8.24-1")' "$stage/builds.json" > /dev/null
done
make -C "$owner" validate
curl -fLsS --proto '=https' --proto-redir '=https' --retry 3 \
 https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/srpm-builds/11240232/uke-boot-integration-1.8.23-1.fc46.src.rpm -o "$stage/base.src.rpm"
printf '%s  %s\n' a988b1c8d6ec3464d771d08da8fc02fe9f2c8f46c128cca544ce8ff3d1656e82 "$stage/base.src.rpm" | sha256sum -c -
rpm2cpio "$stage/base.src.rpm" > "$stage/base.cpio"
cpio -t --quiet < "$stage/base.cpio" | awk '/^\/|(^|\/)\.\.($|\/)/ {bad=1} END {exit bad}'
mkdir -p "$stage/sources" "$stage/unpacked" "$stage/inner" "$output"
(cd "$stage/sources"; cpio -idm --quiet --no-absolute-filenames < "$stage/base.cpio")
archive=$stage/sources/uke-boot-integration-1.8.23.tar.xz
tar -tJf "$archive" | awk '/^\/|(^|\/)\.\.($|\/)/ {bad=1} END {exit bad}'
tar -xJf "$archive" -C "$stage/unpacked"
outer=$stage/unpacked/uke-boot-integration-1.8.23
(cd "$outer"; sha256sum --check --strict --quiet SOURCE-SHA256SUMS)
input=$outer/inputs/uke-boot-integration
archive=$input/uke-boot-integration-1.8.23.tar.xz
tar -tJf "$archive" | awk '/^\/|(^|\/)\.\.($|\/)/ {bad=1} END {exit bad}'
tar -xJf "$archive" -C "$stage/inner"
inner=$stage/inner/uke-boot-integration-1.8.23
for relative in src/units/uke-userdata-accept.service tests/kernel-update.cpp tests/userdata-update.sh src/installation-policy.hpp src/units/uke-shutdown-trace.service src/shutdown-trace.sh tests/shutdown-trace.sh src/install-profile.cpp src/profiles/73-uke-shutdown-trace.rules tests/native-profile-udev.cpp tests/native-profile-udev.sh docs/SHUTDOWN-TRACE.md; do
 cmp "$inner/$relative" "$owner/$relative"
done
for relative in src/kernel-update.cpp src/userdata-update.hpp src/touch-health.hpp tests/touch-health.cpp docs/KERNEL-UPDATES.md; do
 install -m644 "$owner/$relative" "$inner/$relative"
done
rewrite_spec() {
 awk '/^%changelog/ {print;print "* Sun Nov 01 2026 MCC45TR <75160848+MCC45TR@users.noreply.github.com> - 1.8.24-1";print "- Poll strict native touch readiness briefly during packaged startup acceptance.";print "- Preserve all other acceptance, retry, rollback and passive trace behavior.";print "";history=1;next}
      !history {gsub(/1[.]8[.]23-1/,"1.8.24-1");gsub(/1[.]8[.]23/,"1.8.24")}
      {print}' "$1" > "$2"
 test "$(grep -Fc '(libstdc++%{?_isa} >= 16.2.1 or uke-libstdcxx%{?_isa} >= 16.2.1)' "$2")" = 4
 test "$(grep -Fxc 'BuildRequires: glib2 strace systemd' "$2")" = 1
 test "$(grep -Fxc 'install -Dm644 src/profiles/73-uke-shutdown-trace.rules %{buildroot}%{_udevrulesdir}/73-uke-shutdown-trace.rules' "$2")" = 1
 test "$(grep -Fxc '%{_udevrulesdir}/73-uke-shutdown-trace.rules' "$2")" = 1
}
for spec in "$inner/packaging/uke-boot-integration.spec" "$input/uke-boot-integration.spec"; do
 rewrite_spec "$spec" "$stage/updated.spec"
 install -m644 "$stage/updated.spec" "$spec"
done
rewrite_spec "$stage/sources/uke-boot-integration.spec" "$output/uke-boot-integration.spec"
awk '/^%changelog/{exit} /^Requires: uke-(boot-integration|installation-profile) =/ {
 if($4!="1.8.24-1.fc46")exit 1;n++} END{if(n!=5)exit 1}' "$output/uke-boot-integration.spec"
commit=$(git -C "$owner" rev-parse HEAD)
epoch=$(git -C "$owner" show -s --format=%ct HEAD)
mv "$inner" "$stage/inner/uke-boot-integration-1.8.24"
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf "$input/uke-boot-integration-1.8.24.tar.xz" -C "$stage/inner" uke-boot-integration-1.8.24
unlink -- "$input/uke-boot-integration-1.8.23.tar.xz"
jq -n --arg commit "$commit" '{source_commit:$commit,parent_build:11240232,parent_srpm_sha256:"a988b1c8d6ec3464d771d08da8fc02fe9f2c8f46c128cca544ce8ff3d1656e82",functional_changes:["src/kernel-update.cpp","src/userdata-update.hpp","src/touch-health.hpp"],startup_only:true,poll_window_milliseconds:2000,poll_interval_milliseconds:100,unit_and_fallback_unchanged:true,trace_implementation_unchanged:true,physical_acceptance:false}' > "$outer/touch-readiness-overlay.json"
(cd "$outer"; find . -type f ! -name SOURCE-SHA256SUMS -print0 | sort -z | xargs -0 sha256sum > SOURCE-SHA256SUMS)
mv "$outer" "$stage/unpacked/uke-boot-integration-1.8.24"
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf "$output/uke-boot-integration-1.8.24.tar.xz" -C "$stage/unpacked" uke-boot-integration-1.8.24
test "$(rpmspec --target aarch64 --define 'dist %{nil}' -q --srpm --qf '%{VERSION}-%{RELEASE}' "$output/uke-boot-integration.spec")" = 1.8.24-1.fc46
