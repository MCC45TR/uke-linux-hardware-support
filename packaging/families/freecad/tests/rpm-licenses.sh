#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Exercise the real %license file-list syntax without compiling FreeCAD.
set -Eeuo pipefail
root=$(cd -- "$(dirname -- "$0")/.."; pwd)
stage=$(mktemp -d)
trap 'rm -rf -- "$stage"' EXIT
trap 'status=$?; tail -n 35 "$stage/build.log" >&2; exit "$status"' ERR
mkdir -p "$stage/rpm/"{SOURCES,SPECS,BUILD,BUILDROOT,RPMS,SRPMS}
printf 'Synthetic JSON license fixture.\n' >"$stage/rpm/SOURCES/nlohmann-json-LICENSE.MIT"
awk '/^%files$/ {files=1; next} /^%files/ {files=0} files && /^%license / {print}' \
  "${FREECAD_LICENSE_SPEC:-$root/freecad.spec}" >"$stage/licenses"
[[ $(wc -l <"$stage/licenses") == 1 ]]
cat >"$stage/rpm/SPECS/fixture.spec" <<'SPEC'
Name: freecad-license-fixture
Version: 1
Release: 1
Summary: Local FreeCAD RPM license regression fixture
License: MIT
BuildArch: noarch
Source2: nlohmann-json-LICENSE.MIT
%description
Synthetic packaging fixture, never submitted or published.
%prep
mkdir -p src/3rdParty/libkdtree
printf 'Synthetic main license fixture.\n' > LICENSE
printf 'Synthetic kdtree license fixture.\n' > src/3rdParty/libkdtree/COPYING
install -pm0644 %{SOURCE2} nlohmann-json-LICENSE.MIT
%install
install -Dm0644 /dev/null %{buildroot}%{_datadir}/freecad-license-fixture/marker
%files
SPEC
cat "$stage/licenses" >>"$stage/rpm/SPECS/fixture.spec"
printf '%%{_datadir}/freecad-license-fixture/marker\n' >>"$stage/rpm/SPECS/fixture.spec"
rpmbuild -bb --nodeps --define "_topdir $stage/rpm" \
  "$stage/rpm/SPECS/fixture.spec" >"$stage/build.log" 2>&1
rpm -qp --queryformat '[%{FILENAMES}|%{FILEFLAGS:fflags}|%{FILEDIGESTS}\n]' \
  "$stage/rpm/RPMS/noarch/"*.rpm >"$stage/files"
[[ $(awk -F '|' '$2 ~ /l/ {count++} END {print count+0}' "$stage/files") == 3 ]]
expected=$(sha256sum "$stage/rpm/SOURCES/nlohmann-json-LICENSE.MIT" | cut -d ' ' -f1)
actual=$(awk -F '|' '$1 ~ /\/nlohmann-json-LICENSE[.]MIT$/ && $2 ~ /l/ {print $3}' "$stage/files")
[[ $actual == "$expected" ]]
printf '%s\n' 'RPM packages all three license texts with license flags and preserves the JSON license digest.'
