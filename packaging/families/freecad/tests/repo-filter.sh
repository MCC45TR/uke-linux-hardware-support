#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Real RPM/DNF fixtures; no FreeCAD compilation, network repositories or payload installation.
set -Eeuo pipefail
root=$(cd -- "$(dirname -- "$0")/.."; pwd)
stage=$(mktemp -d)
trap 'rm -rf -- "$stage"' EXIT
trap 'status=$?; find "$stage" -maxdepth 1 -name "*.log" -exec tail -n 25 {} \; >&2; exit "$status"' ERR
package=$(jq -er '.name|select(.=="uke-freecad")' "$root/source-lock.json")
epoch=$(jq -er .epoch "$root/source-lock.json")
version=$(jq -er .version "$root/source-lock.json")
release=$(jq -er .release "$root/source-lock.json")
evr="$epoch:$version-$release"
top="$stage/rpm"
mkdir -p "$top/"{SOURCES,SPECS,BUILD,BUILDROOT,RPMS,SRPMS} "$stage/repo" "$stage/no-repos"
rpmspec --target aarch64 --define 'dist %{nil}' --query --queryformat \
  'NAME=%{NAME}\n[Provides: %{PROVIDENAME} %{PROVIDEFLAGS:depflags} %{PROVIDEVERSION}\n][Obsoletes: %{OBSOLETENAME} %{OBSOLETEFLAGS:depflags} %{OBSOLETEVERSION}\n][Conflicts: %{CONFLICTNAME} %{CONFLICTFLAGS:depflags} %{CONFLICTVERSION}\n][Requires: %{REQUIRENAME} %{REQUIREFLAGS:depflags} %{REQUIREVERSION}\n]' \
  "$root/freecad.spec" >"$stage/headers"
headers() {
  awk -v name="$1" '/^NAME=/ {selected=($0=="NAME="name); next} selected' "$stage/headers" |
    awk '/^(Provides|Obsoletes|Conflicts):/ || /^Requires: uke-freecad-data /'
}
{
  printf 'Name: %s\nEpoch: %s\nVersion: %s\nRelease: %s\nSummary: Synthetic FreeCAD rename fixture\nLicense: MIT\n' "$package" "$epoch" "$version" "$release"
  headers "$package"
  printf '%%description\nLocal packaging fixture, never published or executed.\n%%package data\nSummary: Synthetic data fixture\nBuildArch: noarch\n'
  headers "$package-data"
  cat <<'SPEC'
%description data
Local packaging fixture, never published or executed.
%install
install -Dm0755 /dev/null %{buildroot}/usr/bin/FreeCAD
install -Dm0755 /dev/null %{buildroot}/usr/bin/FreeCADCmd
install -Dm0644 /dev/null %{buildroot}/usr/share/freecad-filter-fixture/data
%files
/usr/bin/FreeCAD
/usr/bin/FreeCADCmd
%files data
/usr/share/freecad-filter-fixture/data
SPEC
} >"$top/SPECS/new.spec"
rpmbuild -bb --nodeps --target aarch64 --define "_topdir $top" "$top/SPECS/new.spec" >"$stage/new-build.log" 2>&1
legacy() {
  local rel=$1
  cat >"$top/SPECS/legacy.spec" <<SPEC
Name: freecad
Epoch: $epoch
Version: $version
Release: $rel
Summary: Synthetic legacy FreeCAD fixture
License: MIT
Requires: freecad-data = $epoch:$version-$rel
%description
Local packaging fixture, never published or executed.
%package data
Summary: Synthetic legacy data fixture
BuildArch: noarch
%description data
Local packaging fixture, never published or executed.
%install
install -Dm0755 /dev/null %{buildroot}/usr/bin/FreeCAD
install -Dm0755 /dev/null %{buildroot}/usr/bin/FreeCADCmd
install -Dm0644 /dev/null %{buildroot}/usr/share/freecad-filter-fixture/data
%files
/usr/bin/FreeCAD
/usr/bin/FreeCADCmd
%files data
/usr/share/freecad-filter-fixture/data
SPEC
  rpmbuild -bb --nodeps --target aarch64 --define "_topdir $top" "$top/SPECS/legacy.spec" >"$stage/legacy-$rel.log" 2>&1
}
legacy "$((release-1))"
legacy "$release"
legacy "$((release+1))"
cat >"$top/SPECS/consumer.spec" <<SPEC
Name: uke-freecad-consumer-fixture
Version: 1
Release: 1
Summary: Synthetic consumer of FreeCAD compatibility names
License: MIT
BuildArch: noarch
Requires: freecad = $evr
Requires: freecad(aarch-64) = $evr
Requires: freecad-data = $evr
%description
Local packaging fixture, never published or executed.
%install
install -Dm0644 /dev/null %{buildroot}/usr/share/freecad-filter-fixture/consumer
%files
/usr/share/freecad-filter-fixture/consumer
SPEC
rpmbuild -bb --nodeps --define "_topdir $top" "$top/SPECS/consumer.spec" >"$stage/consumer-build.log" 2>&1
cp "$top/RPMS/aarch64/"*.rpm "$top/RPMS/noarch/"*.rpm "$stage/repo/"
createrepo_c --quiet "$stage/repo"
common=(--forcearch=aarch64 --releasever=rawhide --use-host-config
  --setopt="reposdir=$stage/no-repos" --disable-repo='*'
  --repofrompath="uke-filter,file://$stage/repo" --enable-repo=uke-filter
  --setopt=uke-filter.gpgcheck=False --setopt=uke-filter.skip_if_unavailable=False
  '--setopt=uke-filter.includepkgs=uke-*,senemos-uke-linux-kernel-*')
LC_ALL=C dnf5 "${common[@]}" --installroot="$stage/query-root" repoquery --available --queryformat '%{name}\n' >"$stage/visible" 2>"$stage/query.log"
if grep -Ex 'freecad|freecad-data' "$stage/visible"; then
  echo 'The Uke include filter exposed an unprefixed RPM name.' >&2; exit 1
fi
grep -Fx "$package" "$stage/visible" >/dev/null
grep -Fx "$package-data" "$stage/visible" >/dev/null
solve() {
  local label=$1 installroot=$2; shift 2
  if LC_ALL=C dnf5 "${common[@]}" --installroot="$installroot" --assumeno "$@" >"$stage/$label.log" 2>&1; then result=0; else result=$?; fi
}
transaction() {
  [[ $result == 1 ]]
  grep -Fx 'Transaction Summary:' "$stage/$1.log" >/dev/null
  grep -Fx 'Operation aborted by the user.' "$stage/$1.log" >/dev/null
  grep -Eq '^[[:space:]]+uke-freecad[[:space:]]+aarch64' "$stage/$1.log"
  grep -Eq '^[[:space:]]+uke-freecad-data[[:space:]]+noarch' "$stage/$1.log"
}
solve fresh "$stage/fresh-root" install "$package"
transaction fresh
solve compatibility "$stage/compatibility-root" install uke-freecad-consumer-fixture
transaction compatibility
seed() {
  local rel=$1 installroot=$2
  mkdir -p "$installroot"
  rpm --root "$installroot" --initdb
  # These locally generated unsigned fixtures seed only an isolated RPM database.
  # Keep digest verification; published COPR signature checks are a separate step.
  rpm --define '_pkgverify_level digest' --root "$installroot" --justdb --nodeps --noscripts --ignorearch -i \
    "$top/RPMS/aarch64/freecad-$version-$rel.aarch64.rpm" "$top/RPMS/noarch/freecad-data-$version-$rel.noarch.rpm"
}
for rel in "$((release-1))" "$release"; do
  seed "$rel" "$stage/upgrade-$rel-root"
  solve "upgrade-$rel" "$stage/upgrade-$rel-root" upgrade
  transaction "upgrade-$rel"
  grep -Eq 'replacing[[:space:]]+freecad[[:space:]]' "$stage/upgrade-$rel.log"
  grep -Eq 'replacing[[:space:]]+freecad-data[[:space:]]' "$stage/upgrade-$rel.log"
done
seed "$((release+1))" "$stage/newer-root"
solve newer "$stage/newer-root" install "$package"
[[ $result != 0 ]]
grep -E 'conflicts with.*freecad|freecad.*conflicts with' "$stage/newer.log" >/dev/null
if grep -Fx 'Transaction Summary:' "$stage/newer.log" >/dev/null; then
  echo 'A newer upstream package was allowed in the Uke transaction.' >&2; exit 1
fi
printf '%s\n' 'AArch64 DNF fixtures passed the exact Uke include filter, fresh install, compatibility consumers, older/equal upgrades and newer-upstream conflict checks.'
