#!/usr/bin/env bash
# COPR only: retain the published frontend/package-manager source composition.
set -Eeuo pipefail
export LC_ALL=C
test ! -e /etc/senemos/kernel-update.conf
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
output=$(realpath -m "${1:?COPR output}")
stage=$(mktemp -d)
trap 'find "$stage" -depth -delete' EXIT
mkdir -p "$output" "$stage/overlay"
curl -fLsS --retry 2 --proto '=https' --proto-redir '=https' \
 https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/srpm-builds/11259667/uke-control-2.8.1-13.fc46.src.rpm -o "$stage/parent.rpm"
printf '%s  %s\n' d1968e50fc7e8a330fdb2a1572794e6ce01667fc7dc5179a67e0444c554ed6c8 "$stage/parent.rpm" | sha256sum -c --strict
rpm2cpio "$stage/parent.rpm" > "$stage/input.cpio"
cpio -t --quiet < "$stage/input.cpio" > "$stage/entries"
awk '/^\/|(^|\/)\.\.($|\/)/ {bad=1} END {exit bad}' "$stage/entries"
(cd "$output"; cpio -idm --quiet --no-absolute-filenames < "$stage/input.cpio")
for path in src/main.cpp src/cli.cpp src/rollback.hpp tests/rollback.cpp packaging/uke-control.bash; do
 install -Dm644 "$owner/$path" "$stage/overlay/$path"
done
epoch=$(git -C "$owner" show -s --format=%ct HEAD)
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf "$output/uke-control-rollback.tar.xz" -C "$stage/overlay" .
sed 's/13[.]fc46/23.fc46/g' "$output/uke-control.spec" > "$stage/base.spec"
awk '
 /^Source5:/ {print "Source6: uke-control-rollback.tar.xz"}
 /^Requires: uke-kernel-update\(transaction-v1\)/ {print "Requires: uke-kernel-update(rollback-policy-v1) >= 1"}
 /^%build$/ {
  print "mapfile -t rollback_sources < <(find %{_builddir}/uke-control-2.8.1/work -type f -path '\''*/src/main.cpp'\'')"
  print "test ${#rollback_sources[@]} = 1"
  print "rollback_root=${rollback_sources[0]%/src/main.cpp}"
  print "tar -xJf %{SOURCE6} -C \"$rollback_root\""
 }
 /^%check$/ {
  print; print "mapfile -t rollback_sources < <(find %{_builddir}/uke-control-2.8.1/work -type f -path '\''*/tests/rollback.cpp'\'')"
  print "test ${#rollback_sources[@]} = 1"
  print "g++ -std=c++20 -O2 -UNDEBUG -Wall -Wextra -Werror \"${rollback_sources[0]}\" -o %{_builddir}/rollback-cli-contract"
  print "%{_builddir}/rollback-cli-contract"; next
 }
 {print}
' "$stage/base.spec" > "$output/uke-control.spec"
[[ $(rpmspec -q --srpm --qf '%{VERSION}-%{RELEASE}' "$output/uke-control.spec") == 2.8.1-23.fc46 ]]
