#!/usr/bin/env bash
# COPR only: retain exact installed22 input and qualify request ownership.
set -Eeuo pipefail
trap 'status=$?; printf "Source preparation failed at line %s (exit %s)\n" "$LINENO" "$status" >&2; exit "$status"' ERR
export LC_ALL=C
test ! -e /etc/senemos/kernel-update.conf
[[ $(rpm --eval '%{_host_cpu}') == aarch64 && $(rpm --eval '%{fedora}') == 46 ]]
root=$(cd -- "$(dirname -- "$0")/.." && pwd)
output=$(realpath -m "${1:?COPR result directory}")
stage=$(mktemp -d)
trap 'find "$stage" -depth -delete' EXIT
for project in uke-linux-unstable uke-linux-test uke-linux; do
 offset=0
 while ((offset < 10000)); do
  curl -fLsS --max-time 45 --get --data-urlencode ownername=mcc45tr \
   --data-urlencode "projectname=$project" --data-urlencode packagename=uke-libssc \
   --data-urlencode limit=100 --data-urlencode "offset=$offset" \
   https://copr.fedorainfracloud.org/api_3/build/list > "$stage/builds.json"
  jq -e '(.items|type)=="array" and all(.items[];
    ((.source_package.version // "")|test("^0[.]4[.]4-23([.]|$)"))|not)' "$stage/builds.json" >/dev/null
  count=$(jq -er '.items|length' "$stage/builds.json")
  ((count < 100)) && break
  offset=$((offset + 100))
 done
 ((offset < 10000))
done
curl -fLsS --max-time 60 --proto '=https' --proto-redir '=https' \
 https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/srpm-builds/11247602/uke-libssc-0.4.4-22.fc46.src.rpm -o "$stage/parent.src.rpm"
printf '%s  %s\n' 90427be2e0f0aa0e173cdbb10462ff83c7e4adbbf28e8fe2596b422f5cdcede1 "$stage/parent.src.rpm" | sha256sum -c -
rpmkeys --checksig --nosignature "$stage/parent.src.rpm"
[[ $(rpm -qp --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$stage/parent.src.rpm") == uke-libssc-0.4.4-22.fc46 ]]
rpm2cpio "$stage/parent.src.rpm" > "$stage/source.cpio"
cpio -t --quiet < "$stage/source.cpio" > "$stage/entries"
awk '/^\/|(^|\/)\.\.($|\/)/ {bad=1} END {exit bad}' "$stage/entries"
mkdir -p "$output"
(cd "$output"; cpio -idm --quiet --no-absolute-filenames < "$stage/source.cpio")
spec=$output/uke-libssc.spec
sha256sum "$output/uke-libssc-0.4.4.tar.xz" > "$stage/archive.sha256"
install -m644 "$root/patches/libssc/0010-own-report-request-lifetime.patch" "$output/"
for input in libssc-request-lifetime.sh libssc-request-lifetime-seam.c libssc-request-lifetime.cpp; do
 install -m644 "$root/tests/$input" "$output/"
done
[[ $(grep -Fxc 'Release: 22.fc46' "$spec") == 2 ]]
[[ $(grep -Fxc 'meson compile -C build' "$spec") == 2 ]]
awk '
 /^%changelog$/ {section="changelog"}
 section!="changelog" && /^(Release:|Provides:|Obsoletes:|Requires:)/ {
  gsub(/0[.]4[.]4-22[.]fc46/,"0.4.4-23.fc46")
  sub(/^Release: 22[.]fc46$/,"Release: 23.fc46")
 }
 /^Source2:/ {
  print
  print "Source3: 0010-own-report-request-lifetime.patch"
  print "Source4: libssc-request-lifetime.sh"
  print "Source5: libssc-request-lifetime-seam.c"
  print "Source6: libssc-request-lifetime.cpp"
  next
 }
 /^meson compile -C build$/ {
  print
  if (++compiled != 2) next
  print "set +e"
  print "bash %{SOURCE4} src/libssc-sensor.c %{SOURCE5} %{SOURCE6} > lifecycle-before.log 2>&1"
  print "lifecycle_before_status=$?"
  print "set -e"
  print "if test \"$lifecycle_before_status\" != 134; then cat lifecycle-before.log; exit 1; fi"
  print "grep -Fq '\''done.calls == 2'\'' lifecycle-before.log"
  print "echo '\''Original parent source loses an overlapping request (expected status134)'\''"
  print "patch --dry-run -p1 --fuzz=0 --no-backup-if-mismatch < %{SOURCE3}"
  print "patch -p1 --fuzz=0 --no-backup-if-mismatch < %{SOURCE3}"
  print "meson compile -C build"
  next
 }
 /^%check$/ {section="check"}
 section=="check" && /^cd "%\{_builddir\}\/uke-libssc-0[.]4[.]4\/work\/libssc\/libssc"$/ {
  print
  print "bash %{SOURCE4} src/libssc-sensor.c %{SOURCE5} %{SOURCE6}"
  print "SANITIZE=1 bash %{SOURCE4} src/libssc-sensor.c %{SOURCE5} %{SOURCE6}"
  next
 }
 /^%changelog$/ {
  print
  print "* Tue Nov 03 2026 MCC45TR <75160848+MCC45TR@users.noreply.github.com> - 0.4.4-23.fc46"
  print "- Own each SSC report subscription and task independently."
  print "- Qualify actual-source overlapping and delayed acknowledgement lifetimes."
  print ""
  next
 }
 {print}
' "$spec" > "$stage/next.spec"
mv "$stage/next.spec" "$spec"
sha256sum -c "$stage/archive.sha256"
[[ $(grep -Fxc 'meson compile -C build' "$spec") == 3 ]]
[[ $(grep -Fxc 'bash %{SOURCE4} src/libssc-sensor.c %{SOURCE5} %{SOURCE6}' "$spec") == 1 ]]
[[ $(rpmspec --target aarch64 -q --srpm --qf '%{VERSION}-%{RELEASE}' "$spec") == 0.4.4-23.fc46 ]]
