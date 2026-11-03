#!/usr/bin/env bash
# COPR-only: retain the actual release21 recipe and fix its sample-rate GType.
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
    ((.source_package.version // "")|test("^0[.]4[.]4-22([.]|$)"))|not)' "$stage/builds.json" >/dev/null
  count=$(jq -er '.items|length' "$stage/builds.json")
  ((count < 100)) && break
  offset=$((offset + 100))
 done
 ((offset < 10000))
done
# Source21 was prepared by11247525; its first and only binary build is11247548.
curl -fLsS --max-time 60 --proto '=https' --proto-redir '=https' \
 https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/srpm-builds/11247525/uke-libssc-0.4.4-21.fc46.src.rpm -o "$stage/parent.src.rpm"
printf '%s  %s\n' bed2cb5f80ebe18282a9246ff75e6b7f710afb64b3f5acb20d60ddc4c607b83c "$stage/parent.src.rpm" | sha256sum -c -
rpmkeys --checksig --nosignature "$stage/parent.src.rpm"
[[ $(rpm -qp --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$stage/parent.src.rpm") == uke-libssc-0.4.4-21.fc46 ]]
rpm2cpio "$stage/parent.src.rpm" > "$stage/source.cpio"
cpio -t --quiet < "$stage/source.cpio" > "$stage/entries"
awk '/^\/|(^|\/)\.\.($|\/)/ {bad=1} END {exit bad}' "$stage/entries"
mkdir -p "$output"
(cd "$output"; cpio -idm --quiet --no-absolute-filenames < "$stage/source.cpio")
spec=$output/uke-libssc.spec
sha256sum "$output/uke-libssc-0.4.4.tar.xz" > "$stage/archive.sha256"
install -m644 "$root/patches/libssc/0009-correct-sample-rate-property-type.patch" "$output/"
install -m644 "$root/tests/libssc-sample-rate-property.cpp" "$output/"
[[ $(grep -Fxc 'Release: 21.fc46' "$spec") == 2 ]]
[[ $(grep -Fxc 'meson compile -C build' "$spec") == 1 ]]
[[ $(grep -Fxc 'cd "%{_builddir}/uke-libssc-0.4.4/work/libssc/libssc"' "$spec") == 3 ]]
awk '
 /^%changelog$/ {section="changelog"}
 section!="changelog" && /^(Release:|Provides:|Obsoletes:|Requires:)/ {
  gsub(/0[.]4[.]4-21[.]fc46/,"0.4.4-22.fc46")
  sub(/^Release: 21[.]fc46$/,"Release: 22.fc46")
 }
 /^Source0:/ {
  print
  print "Source1: 0009-correct-sample-rate-property-type.patch"
  print "Source2: libssc-sample-rate-property.cpp"
  next
 }
 /^meson compile -C build$/ {
  print
  print "g++ -std=c++20 -UNDEBUG -Wall -Wextra -Werror -I src %{SOURCE2} $(pkg-config --cflags --libs gio-2.0) -L build/src -lssc -o sample-rate-property-test"
  print "ulimit -c 0"
  print "set +e"
  print "LD_LIBRARY_PATH=\"$PWD/build/src\" G_DEBUG=fatal-criticals ./sample-rate-property-test > sample-rate-before.log 2>&1"
  print "sample_rate_before_status=$?"
  print "set -e"
  print "test \"$sample_rate_before_status\" = 134"
  print "grep -Fq G_TYPE_FLOAT sample-rate-before.log"
  print "echo '\''Original real GObject property fails its float contract (expected status 134)'\''"
  print "patch --dry-run -p1 --fuzz=0 --no-backup-if-mismatch < %{SOURCE1}"
  print "patch -p1 --fuzz=0 --no-backup-if-mismatch < %{SOURCE1}"
  print "meson compile -C build"
  next
 }
 /^%check$/ {section="check"}
 section=="check" && /^cd "%\{_builddir\}\/uke-libssc-0[.]4[.]4\/work\/libssc\/libssc"$/ {
  print
  print "LD_LIBRARY_PATH=\"$PWD/build/src\" G_DEBUG=fatal-criticals ./sample-rate-property-test"
  print "g++ -std=c++20 -UNDEBUG -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer -I src %{SOURCE2} $(pkg-config --cflags --libs gio-2.0) -L build/src -lssc -o sample-rate-property-sanitized"
  print "LD_LIBRARY_PATH=\"$PWD/build/src\" G_DEBUG=fatal-criticals ./sample-rate-property-sanitized"
  next
 }
 /^%changelog$/ {
  print
  print "* Tue Nov 03 2026 MCC45TR <75160848+MCC45TR@users.noreply.github.com> - 0.4.4-22.fc46"
  print "- Match the sample-rate property type to its float getter."
  print "- Retain source21 and verify the real GObject contract before and after the fix."
  print ""
  next
 }
 {print}
' "$spec" > "$stage/next.spec"
mv "$stage/next.spec" "$spec"
sha256sum -c "$stage/archive.sha256"
[[ $(grep -Fxc 'LD_LIBRARY_PATH="$PWD/build/src" G_DEBUG=fatal-criticals ./sample-rate-property-test' "$spec") == 1 ]]
[[ $(grep -Fxc 'LD_LIBRARY_PATH="$PWD/build/src" G_DEBUG=fatal-criticals ./sample-rate-property-sanitized' "$spec") == 1 ]]
[[ $(rpmspec --target aarch64 -q --srpm --qf '%{VERSION}-%{RELEASE}' "$spec") == 0.4.4-22.fc46 ]]
