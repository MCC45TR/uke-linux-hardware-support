#!/usr/bin/env bash
# COPR only: retain installed23 and own queued measurement contexts.
set -Eeuo pipefail
trap 'status=$?; printf "Measurement source preparation failed at line %s (exit %s)\n" "$LINENO" "$status" >&2; exit "$status"' ERR
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
    ((.source_package.version // "")|test("^0[.]4[.]4-26([.]|$)"))|not)' "$stage/builds.json" >/dev/null
  count=$(jq -er '.items|length' "$stage/builds.json")
  ((count < 100)) && break
  offset=$((offset + 100))
 done
 ((offset < 10000))
done
curl -fLsS --max-time 60 --proto '=https' --proto-redir '=https' \
 https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/srpm-builds/11250524/uke-libssc-0.4.4-23.fc46.src.rpm -o "$stage/parent.src.rpm"
printf '%s  %s\n' e6952a37ae1f55c1f0078c3c8aabfc3d639266f51a51f8f7754371dc7fc295aa "$stage/parent.src.rpm" | sha256sum -c -
rpmkeys --checksig --nosignature "$stage/parent.src.rpm"
[[ $(rpm -qp --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$stage/parent.src.rpm") == uke-libssc-0.4.4-23.fc46 ]]
rpm2cpio "$stage/parent.src.rpm" > "$stage/source.cpio"
cpio -t --quiet < "$stage/source.cpio" > "$stage/entries"
awk '/^\/|(^|\/)\.\.($|\/)/ {bad=1} END {exit bad}' "$stage/entries"
mkdir -p "$output"
(cd "$output"; cpio -idm --quiet --no-absolute-filenames < "$stage/source.cpio")
spec=$output/uke-libssc.spec
find "$output" -maxdepth 1 -type f ! -name uke-libssc.spec -print0 | sort -z | xargs -0 sha256sum > "$stage/retained.sha256"
install -m644 "$root/patches/libssc/0011-own-queued-measurement-context.patch" "$output/"
for input in libssc-measurement-lifetime.sh libssc-measurement-lifetime-seam.c libssc-measurement-lifetime.cpp libssc-proximity-values.sh; do
 install -m644 "$root/tests/$input" "$output/"
done
[[ $(grep -Fxc 'Release: 23.fc46' "$spec") == 2 ]]
[[ $(grep -Fxc 'meson compile -C build' "$spec") == 3 ]]
awk '
 /^%changelog$/ {section="changelog"}
 section!="changelog" && /^(Release:|Provides:|Obsoletes:|Requires:)/ {
  gsub(/0[.]4[.]4-23[.]fc46/,"0.4.4-26.fc46")
  sub(/^Release: 23[.]fc46$/,"Release: 26.fc46")
 }
 /^Source6:/ {
  print
  print "Source7: 0011-own-queued-measurement-context.patch"
  print "Source8: libssc-measurement-lifetime.sh"
  print "Source9: libssc-measurement-lifetime-seam.c"
  print "Source10: libssc-measurement-lifetime.cpp"
  print "Source11: libssc-proximity-values.sh"
  next
 }
 /^meson compile -C build$/ {
  print
  if (++compiled != 3) next
  print "set +e"
  print "EXPECT_UNOWNED=1 bash %{SOURCE8} src %{SOURCE9} %{SOURCE10} > measurement-before.log 2>&1"
  print "measurement_before_status=$?"
  print "set -e"
  print "if test \"$measurement_before_status\" != 134; then cat measurement-before.log; exit 1; fi"
  print "grep -Fq '\''All six original measurement classes fail pending ownership'\'' measurement-before.log"
  print "cat measurement-before.log"
  print "patch --dry-run -p1 --fuzz=0 --no-backup-if-mismatch < %{SOURCE7}"
  print "patch -p1 --fuzz=0 --no-backup-if-mismatch < %{SOURCE7}"
  print "meson compile -C build"
  next
 }
 /^%check$/ {section="check"}
 section=="check" && /^cd "%\{_builddir\}\/uke-libssc-0[.]4[.]4\/work\/libssc\/libssc"$/ {
  print
  print "bash %{SOURCE8} src %{SOURCE9} %{SOURCE10}"
  print "SANITIZE=1 bash %{SOURCE8} src %{SOURCE9} %{SOURCE10}"
  next
 }
 /^%changelog$/ {
  print
  print "* Tue Nov 03 2026 MCC45TR <75160848+MCC45TR@users.noreply.github.com> - 0.4.4-26.fc46"
  print "- Own sensors until queued measurement dispatch or cancellation completes."
  print "- Qualify all six real measurement classes and retain prior regressions."
  print ""
  next
 }
 {
  gsub(/%\{_builddir\}\/uke-libssc-0[.]4[.]4\/inputs\/libssc\/libssc-proximity-values[.]sh/,"%{SOURCE11}")
  print
 }
' "$spec" > "$stage/next.spec"
mv "$stage/next.spec" "$spec"
sha256sum -c "$stage/retained.sha256"
[[ $(grep -Fxc 'meson compile -C build' "$spec") == 4 ]]
[[ $(grep -Fxc 'bash %{SOURCE8} src %{SOURCE9} %{SOURCE10}' "$spec") == 1 ]]
[[ $(grep -Fxc 'SANITIZE=1 bash %{SOURCE4} src/libssc-sensor.c %{SOURCE5} %{SOURCE6}' "$spec") == 1 ]]
[[ $(rpmspec --target aarch64 -q --srpm --qf '%{VERSION}-%{RELEASE}' "$spec") == 0.4.4-26.fc46 ]]
