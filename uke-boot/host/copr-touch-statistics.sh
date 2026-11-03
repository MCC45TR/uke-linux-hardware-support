#!/usr/bin/env bash
# COPR only: retain qualified37 and admit additive native touch statistics.
set -Eeuo pipefail
export LC_ALL=C
test ! -e /etc/senemos/kernel-update.conf
[[ $(rpm --eval '%{_host_cpu}') == aarch64 && $(rpm --eval '%{fedora}') == 46 ]]
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
output=$(realpath -m "${1:?COPR result directory}")
make -C "$owner" validate
stage=$(mktemp -d)
trap 'find "$stage" -depth -delete' EXIT
for project in uke-linux-unstable uke-linux-test uke-linux; do
 curl -fLsS --get --data-urlencode ownername=mcc45tr --data-urlencode "projectname=$project" \
  --data-urlencode packagename=uke-boot-integration --data-urlencode limit=100 \
  https://copr.fedorainfracloud.org/api_3/build/list > "$stage/builds.json"
 jq -e '(.items|type)=="array" and (.items|length)<100 and all(.items[];
   (.source_package.version // "") as $version |
   if $version=="" then true
   elif ($version|test("^[0-9]+[.][0-9]+[.][0-9]+-")) then
     (($version|capture("^(?<a>[0-9]+)[.](?<b>[0-9]+)[.](?<c>[0-9]+)-")|[.a,.b,.c]|map(tonumber)) < [1,8,38])
   else false end)' "$stage/builds.json" >/dev/null
done
curl -fLsS --retry 2 --proto '=https' --proto-redir '=https' \
 https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/srpm-builds/11251295/uke-boot-integration-1.8.37-1.fc46.src.rpm \
 -o "$stage/base.src.rpm"
parent=d5ff2a79b13edb64d8e8ce4276e4d68f5bcf5ee5900d95516bedc1cbe7961d2f
printf '%s  %s\n' "$parent" "$stage/base.src.rpm" | sha256sum -c -
rpmkeys --checksig --nosignature "$stage/base.src.rpm"
[[ $(rpm -qp --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$stage/base.src.rpm") == uke-boot-integration-1.8.37-1.fc46 ]]
rpm2cpio "$stage/base.src.rpm" > "$stage/base.cpio"
cpio -t --quiet < "$stage/base.cpio" | awk '/^\/|(^|\/)\.\.($|\/)/ {bad=1} END {exit bad}'
mkdir -p "$stage/sources" "$stage/outer" "$stage/inner" "$output"
(cd "$stage/sources"; cpio -idm --quiet --no-absolute-filenames < "$stage/base.cpio")
unpack() {
 tar -tJf "$1" | awk '/^\/|(^|\/)\.\.($|\/)/ {bad=1} END {exit bad}'
 tar -xJf "$1" -C "$2"
}
unpack "$stage/sources/uke-boot-integration-1.8.37.tar.xz" "$stage/outer"
outer=$stage/outer/uke-boot-integration-1.8.37
(cd "$outer"; sha256sum --check --strict --quiet SOURCE-SHA256SUMS)
input=$outer/inputs/uke-boot-integration
unpack "$input/uke-boot-integration-1.8.37.tar.xz" "$stage/inner"
inner=$stage/inner/uke-boot-integration-1.8.37
(cd "$inner"; find . -type f -print0 | sort -z | xargs -0 sha256sum) > "$stage/before.sha256"
printf '%s  %s\n' b19d00445bcaed98068554d79c93e8843efa070666e37d1044e6e242a408ba01 "$inner/src/touch-health.hpp" | sha256sum -c -
install -Dm644 "$inner/src/touch-health.hpp" "$inner/tests/fixtures/touch-health-before-stats.hpp"
cat > "$stage/overlays" <<'FILES'
Makefile
src/touch-health.hpp
tests/touch-health.cpp
docs/KERNEL-UPDATES.md
packaging/uke-boot-integration.spec
FILES
while read -r path; do
 [[ $path == packaging/uke-boot-integration.spec ]] || install -Dm644 "$owner/$path" "$inner/$path"
done < "$stage/overlays"
rewrite_spec() {
 awk '
 /^%changelog/ {history=1}
 !history {gsub(/1[.]8[.]37/,"1.8.38")}
 {print}
 /^Provides: uke-kernel-update\(trial-health-v2\) = 1$/ {print "Provides: uke-kernel-update(touch-status-v2) = 1"}
 ' "$1" > "$2"
}
for spec in "$inner/packaging/uke-boot-integration.spec" "$input/uke-boot-integration.spec"; do
 rewrite_spec "$spec" "$stage/updated.spec";install -m644 "$stage/updated.spec" "$spec"
done
while read -r hash path; do
 path=${path#./}
 if ! grep -Fxq "$path" "$stage/overlays"; then printf '%s  %s\n' "$hash" "$inner/$path" | sha256sum -c --quiet -;fi
done < "$stage/before.sha256"
rewrite_spec "$stage/sources/uke-boot-integration.spec" "$output/uke-boot-integration.spec"
awk '
 /^%build$/ {section="build"}
 /^%install$/ {section="install"}
 /^%check$/ {section="check"}
 /^Source0:/ {print;print "BuildRequires: libasan libubsan";next}
 section=="build" && /^cd "/ {
  print
  print "ulimit -c 0"
  print "g++ -std=c++20 -O2 -UNDEBUG -Wall -Wextra -Werror -I src \"-DUKE_TOUCH_HEALTH_SOURCE=\\\"$PWD/tests/fixtures/touch-health-before-stats.hpp\\\"\" tests/touch-health.cpp -o touch-health-before"
  print "set +e"
  print "./touch-health-before > touch-health-before.log 2>&1"
  print "touch_health_status=$?"
  print "set -e"
  print "if test \"$touch_health_status\" != 134; then cat touch-health-before.log; exit 1; fi"
  print "grep -Fq '\''Additive touch statistics must retain readiness admission'\'' touch-health-before.log"
  print "echo '\''Original touch-health consumer rejects additive statistics (expected status134)'\''"
  next
 }
 section=="check" && /^\.\/touch-health-contract$/ {
  print
  print "g++ -std=c++20 -O1 -g -UNDEBUG -Wall -Wextra -Werror -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer tests/touch-health.cpp -o touch-health-sanitized"
  print "ASAN_OPTIONS=detect_leaks=1:abort_on_error=1 UBSAN_OPTIONS=halt_on_error=1 ./touch-health-sanitized"
  next
 }
 {print}
' "$output/uke-boot-integration.spec" > "$stage/checked.spec"
mv "$stage/checked.spec" "$output/uke-boot-integration.spec"
[[ $(grep -Fc 'ASAN_OPTIONS=detect_leaks=1:abort_on_error=1' "$output/uke-boot-integration.spec") == 1 ]]
[[ $(grep -Fc 'DUKE_TOUCH_HEALTH_SOURCE=' "$output/uke-boot-integration.spec") == 1 ]]
[[ $(grep -Fc 'migrate-automatic-policy || exit 1' "$output/uke-boot-integration.spec") == 1 ]]
[[ $(grep -Fc 'bash tests/automatic-policy-rpm.sh' "$output/uke-boot-integration.spec") == 1 ]]
[[ $(grep -Fxc './trial-health-contract' "$output/uke-boot-integration.spec") == 1 ]]
commit=$(git -C "$owner" rev-parse HEAD)
epoch=$(git -C "$owner" show -s --format=%ct HEAD)
mv "$inner" "$stage/inner/uke-boot-integration-1.8.38"
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf "$input/uke-boot-integration-1.8.38.tar.xz" -C "$stage/inner" uke-boot-integration-1.8.38
unlink "$input/uke-boot-integration-1.8.37.tar.xz"
jq -n --arg commit "$commit" --arg parent "$parent" \
 '{schema:1,source_commit:$commit,parent_build:11251295,parent_srpm_sha256:$parent,
  native_touch_statistics:"named-ready-and-optional-fault-flags",
  changed_sources:["src/touch-health.hpp","tests/touch-health.cpp"],
  original_header_negative_status:134,sanitized_touch_fixture:true,
  trial_health_policy_unchanged:true,ab_writer_unchanged:true,
  legacy_policy_migration_retained:true,firmware_payload_included:false,physical_acceptance:false}' > "$outer/touch-statistics-overlay.json"
(cd "$outer"; find . -type f ! -name SOURCE-SHA256SUMS -print0 | sort -z | xargs -0 sha256sum > SOURCE-SHA256SUMS)
mv "$outer" "$stage/outer/uke-boot-integration-1.8.38"
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf "$output/uke-boot-integration-1.8.38.tar.xz" -C "$stage/outer" uke-boot-integration-1.8.38
[[ $(rpmspec --target aarch64 --define 'dist %{nil}' -q --srpm --qf '%{VERSION}-%{RELEASE}' "$output/uke-boot-integration.spec") == 1.8.38-1.fc46 ]]
