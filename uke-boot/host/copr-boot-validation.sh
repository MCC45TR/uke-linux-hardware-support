#!/bin/bash
# COPR-only: retain the qualified family and combine startup acceptance/staging.
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
  all(.items[]; ((.source_package.version // "")|sub("[.]fc[0-9]+([.][0-9]+)*$";""))!="1.8.22-1")' "$stage/builds.json" > /dev/null
done
make -C "$owner" validate
curl -fLsS --proto '=https' --proto-redir '=https' --retry 3 \
 https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/srpm-builds/11240029/uke-boot-integration-1.8.21-1.fc46.src.rpm -o "$stage/base.src.rpm"
printf '%s  %s\n' 5351820dcfe84c16993c30bc266507080b18d5af59618e781d689cfcf48b12a1 "$stage/base.src.rpm" | sha256sum -c -
rpm2cpio "$stage/base.src.rpm" > "$stage/base.cpio"
cpio -t --quiet < "$stage/base.cpio" | awk '/^\/|(^|\/)\.\.($|\/)/ {bad=1} END {exit bad}'
mkdir -p "$stage/sources" "$stage/unpacked" "$stage/inner" "$output"
(cd "$stage/sources"; cpio -idm --quiet --no-absolute-filenames < "$stage/base.cpio")
archive=$stage/sources/uke-boot-integration-1.8.21.tar.xz
tar -tJf "$archive" | awk '/^\/|(^|\/)\.\.($|\/)/ {bad=1} END {exit bad}'
tar -xJf "$archive" -C "$stage/unpacked"
outer=$stage/unpacked/uke-boot-integration-1.8.21
(cd "$outer"; sha256sum --check --strict --quiet SOURCE-SHA256SUMS)
input=$outer/inputs/uke-boot-integration
archive=$input/uke-boot-integration-1.8.21.tar.xz
tar -tJf "$archive" | awk '/^\/|(^|\/)\.\.($|\/)/ {bad=1} END {exit bad}'
tar -xJf "$archive" -C "$stage/inner"
inner=$stage/inner/uke-boot-integration-1.8.21
for relative in src/userdata-update.hpp src/installation-policy.hpp src/units/uke-shutdown-trace.service src/shutdown-trace.sh tests/shutdown-trace.sh src/install-profile.cpp; do
 cmp "$inner/$relative" "$owner/$relative"
done
for relative in src/kernel-update.cpp src/units/uke-userdata-accept.service tests/kernel-update.cpp tests/userdata-update.sh docs/KERNEL-UPDATES.md docs/USERDATA-AB.md docs/SHUTDOWN-TRACE.md; do
 install -m644 "$owner/$relative" "$inner/$relative"
done
rewrite_spec() {
 awk '/^%changelog/ {print;print "* Sun Nov 01 2026 MCC45TR <75160848+MCC45TR@users.noreply.github.com> - 1.8.22-1";print "- Run startup acceptance and gated staging under one updater lock.";print "- Reuse the identical running bundle within the operation; retain all admission checks.";print "";history=1;next}
      !history {
        gsub(/1[.]8[.]21-1/,"1.8.22-1")
        gsub(/1[.]8[.]21/,"1.8.22")
      } {print}' "$1" > "$2"
 test "$(grep -Fc '(libstdc++%{?_isa} >= 16.2.1 or uke-libstdcxx%{?_isa} >= 16.2.1)' "$2")" = 4
 test "$(grep -Fxc 'BuildRequires: glib2 strace systemd' "$2")" = 1
}
for spec in "$inner/packaging/uke-boot-integration.spec" "$input/uke-boot-integration.spec"; do
 rewrite_spec "$spec" "$stage/updated.spec"
 install -m644 "$stage/updated.spec" "$spec"
done
rewrite_spec "$stage/sources/uke-boot-integration.spec" "$output/uke-boot-integration.spec"
awk '/^%changelog/{exit} /^Requires: uke-(boot-integration|installation-profile) =/ {
 if($4!="1.8.22-1.fc46")exit 1;n++} END{if(n!=5)exit 1}' "$output/uke-boot-integration.spec"
commit=$(git -C "$owner" rev-parse HEAD)
epoch=$(git -C "$owner" show -s --format=%ct HEAD)
mv "$inner" "$stage/inner/uke-boot-integration-1.8.22"
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf "$input/uke-boot-integration-1.8.22.tar.xz" -C "$stage/inner" uke-boot-integration-1.8.22
unlink -- "$input/uke-boot-integration-1.8.21.tar.xz"
jq -n --arg commit "$commit" '{source_commit:$commit,parent_build:11240029,parent_srpm_sha256:"5351820dcfe84c16993c30bc266507080b18d5af59618e781d689cfcf48b12a1",functional_changes:["src/kernel-update.cpp","src/units/uke-userdata-accept.service"],single_process_boot:true,acceptance_before_activation_policy:true,same_operation_bundle_reuse:true,native_writer_unchanged:true,trace_implementation_unchanged:true,runtime_migration_required:false,physical_acceptance:false}' > "$outer/boot-flow-overlay.json"
(cd "$outer"; find . -type f ! -name SOURCE-SHA256SUMS -print0 | sort -z | xargs -0 sha256sum > SOURCE-SHA256SUMS)
mv "$outer" "$stage/unpacked/uke-boot-integration-1.8.22"
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf "$output/uke-boot-integration-1.8.22.tar.xz" -C "$stage/unpacked" uke-boot-integration-1.8.22
test "$(rpmspec --target aarch64 --define 'dist %{nil}' -q --srpm --qf '%{VERSION}-%{RELEASE}' "$output/uke-boot-integration.spec")" = 1.8.22-1.fc46
