#!/bin/bash
# COPR-only: retain the qualified family and start passive tracing on role-device add.
set -Eeuo pipefail
export LC_ALL=C
test ! -e /etc/senemos/kernel-update.conf
[[ $(rpm --eval '%{_host_cpu}') == aarch64 && $(rpm --eval '%{fedora}') == 46 ]]
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
output=$(realpath -m "${1:?COPR result directory}")
stage=$(mktemp -d)
trap 'find "$stage" -depth -delete' EXIT
# Isolated watchdog lanes use a different version series; exact NVR reuse is refused.
for project in uke-linux-unstable uke-linux-test uke-linux; do
 curl --fail --silent --show-error --max-time 45 --get --data-urlencode ownername=mcc45tr \
  --data-urlencode "projectname=$project" --data-urlencode packagename=uke-boot-integration \
  --data-urlencode limit=100 https://copr.fedorainfracloud.org/api_3/build/list > "$stage/builds.json"
 jq -e '(.items|type)=="array" and (.items|length)<100 and
  all(.items[]; ((.source_package.version // "")|sub("[.]fc[0-9]+([.][0-9]+)*$";""))!="1.8.23-1")' "$stage/builds.json" > /dev/null
done
make -C "$owner" validate
curl -fLsS --proto '=https' --proto-redir '=https' --retry 3 \
 https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/srpm-builds/11240204/uke-boot-integration-1.8.22-1.fc46.src.rpm -o "$stage/base.src.rpm"
printf '%s  %s\n' 40fc47dd04c928a798d361a55fc3b255c6b1118592df451f271aea4b2930d30c "$stage/base.src.rpm" | sha256sum -c -
rpm2cpio "$stage/base.src.rpm" > "$stage/base.cpio"
cpio -t --quiet < "$stage/base.cpio" | awk '/^\/|(^|\/)\.\.($|\/)/ {bad=1} END {exit bad}'
mkdir -p "$stage/sources" "$stage/unpacked" "$stage/inner" "$output"
(cd "$stage/sources"; cpio -idm --quiet --no-absolute-filenames < "$stage/base.cpio")
archive=$stage/sources/uke-boot-integration-1.8.22.tar.xz
tar -tJf "$archive" | awk '/^\/|(^|\/)\.\.($|\/)/ {bad=1} END {exit bad}'
tar -xJf "$archive" -C "$stage/unpacked"
outer=$stage/unpacked/uke-boot-integration-1.8.22
(cd "$outer"; sha256sum --check --strict --quiet SOURCE-SHA256SUMS)
input=$outer/inputs/uke-boot-integration
archive=$input/uke-boot-integration-1.8.22.tar.xz
tar -tJf "$archive" | awk '/^\/|(^|\/)\.\.($|\/)/ {bad=1} END {exit bad}'
tar -xJf "$archive" -C "$stage/inner"
inner=$stage/inner/uke-boot-integration-1.8.22
for relative in src/kernel-update.cpp src/units/uke-userdata-accept.service tests/kernel-update.cpp tests/userdata-update.sh src/userdata-update.hpp src/installation-policy.hpp src/units/uke-shutdown-trace.service src/shutdown-trace.sh tests/shutdown-trace.sh src/install-profile.cpp; do
 cmp "$inner/$relative" "$owner/$relative"
done
for relative in src/profiles/73-uke-shutdown-trace.rules tests/native-profile-udev.cpp tests/native-profile-udev.sh docs/SHUTDOWN-TRACE.md; do
 install -m644 "$owner/$relative" "$inner/$relative"
done
rewrite_spec() {
 awk '/^%changelog/ {print;print "* Sun Nov 01 2026 MCC45TR <75160848+MCC45TR@users.noreply.github.com> - 1.8.23-1";print "- Request the passive USB trace on role-device creation using standard udev activation.";print "- Preserve the existing observer and single-process boot implementation.";print "";history=1;next}
      !history {
        gsub(/1[.]8[.]22-1/,"1.8.23-1")
        gsub(/1[.]8[.]22/,"1.8.23")
        sub(/^# Temporary diagnosis for this build, without boot enablement or a reboot[.]$/,"# Keep current-session tracing without retriggering existing role devices.")
      } {print}
      /^install -Dm644 src\/units\/uke-shutdown-trace[.]service / {
        print "install -Dm644 src/profiles/73-uke-shutdown-trace.rules %{buildroot}%{_udevrulesdir}/73-uke-shutdown-trace.rules";installed++
      }
      /^%\{_unitdir\}\/uke-shutdown-trace[.]service$/ {
        print "%{_udevrulesdir}/73-uke-shutdown-trace.rules";listed++
      }
      END {if(installed!=1 || listed!=1)exit 1}' "$1" > "$2"
 test "$(grep -Fc '(libstdc++%{?_isa} >= 16.2.1 or uke-libstdcxx%{?_isa} >= 16.2.1)' "$2")" = 4
 test "$(grep -Fxc 'BuildRequires: glib2 strace systemd' "$2")" = 1
}
for spec in "$inner/packaging/uke-boot-integration.spec" "$input/uke-boot-integration.spec"; do
 rewrite_spec "$spec" "$stage/updated.spec"
 install -m644 "$stage/updated.spec" "$spec"
done
rewrite_spec "$stage/sources/uke-boot-integration.spec" "$output/uke-boot-integration.spec"
awk '/^%changelog/{exit} /^Requires: uke-(boot-integration|installation-profile) =/ {
 if($4!="1.8.23-1.fc46")exit 1;n++} END{if(n!=5)exit 1}' "$output/uke-boot-integration.spec"
commit=$(git -C "$owner" rev-parse HEAD)
epoch=$(git -C "$owner" show -s --format=%ct HEAD)
mv "$inner" "$stage/inner/uke-boot-integration-1.8.23"
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf "$input/uke-boot-integration-1.8.23.tar.xz" -C "$stage/inner" uke-boot-integration-1.8.23
unlink -- "$input/uke-boot-integration-1.8.22.tar.xz"
jq -n --arg commit "$commit" '{source_commit:$commit,parent_build:11240204,parent_srpm_sha256:"40fc47dd04c928a798d361a55fc3b255c6b1118592df451f271aea4b2930d30c",functional_changes:["src/profiles/73-uke-shutdown-trace.rules"],standard_udev_activation:true,boot_implementation_unchanged:true,trace_implementation_unchanged:true,device_control_writes:false,physical_acceptance:false}' > "$outer/trace-activation-overlay.json"
(cd "$outer"; find . -type f ! -name SOURCE-SHA256SUMS -print0 | sort -z | xargs -0 sha256sum > SOURCE-SHA256SUMS)
mv "$outer" "$stage/unpacked/uke-boot-integration-1.8.23"
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf "$output/uke-boot-integration-1.8.23.tar.xz" -C "$stage/unpacked" uke-boot-integration-1.8.23
test "$(rpmspec --target aarch64 --define 'dist %{nil}' -q --srpm --qf '%{VERSION}-%{RELEASE}' "$output/uke-boot-integration.spec")" = 1.8.23-1.fc46
