#!/usr/bin/env bash
# COPR only: exercise the real RPM upgrade/trigger ordering in an isolated rpmdb.
set -Eeuo pipefail
helper=$(realpath -e "${1:?compiled kernel-update-contract fixture}")
root=$(cd -- "$(dirname -- "$0")/.." && pwd)
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
prefix=$work/prefix
config=$prefix/etc/senemos
state=$prefix/var/lib/senemos/kernel-update
top=$work/rpmbuild
mkdir -p "$top"/{SOURCES,SPECS,BUILD,BUILDROOT,RPMS,SRPMS} "$work/rpmdb"
# These are the actual new source hooks with only the host identity/procedure
# entry points replaced by non-device fixture entry points.
awk '/^%post -n uke-kernel-update$/ {copy=1;next} copy && /^%/ {exit} copy {print}' \
 "$root/packaging/uke-boot-integration.spec" > "$work/post"
grep -Fq 'uke-kernel-update migrate-automatic-policy || exit 1' "$work/post"
cat > "$top/SPECS/fixture.spec" <<'SPEC'
Name: uke-policy-fixture-boot
Version: 1
Release: 1
Summary: Isolated automatic policy fixture
License: MIT
BuildArch: noarch
%description
Only temporary files for RPM transaction behavior.
%install
mkdir -p %{buildroot}@PREFIX@/usr/lib/senemos/kernel-updates/fixture
printf 'fixture\n' > %{buildroot}@PREFIX@/usr/lib/senemos/kernel-updates/fixture/manifest
%files
@PREFIX@/usr/lib/senemos/kernel-updates/fixture/manifest
SPEC
sed -i "s|@PREFIX@|$prefix|g" "$top/SPECS/fixture.spec"
rpmbuild -bb --quiet --define "_topdir $top" --define "_rpmdir $top/RPMS" --define "_srcrpmdir $top/SRPMS" --define "_builddir $top/BUILD" --define "_buildrootdir $top/BUILDROOT" --define '_rpmfilename %{ARCH}/%{NAME}-%{VERSION}-%{RELEASE}.%{ARCH}.rpm' --define 'debug_package %{nil}' "$top/SPECS/fixture.spec"
# These unsigned packages exist only in this temporary database. Keep digest
# verification enabled; production packages are signed by COPR after %check.
rpm --dbpath "$work/rpmdb" --nosignature --nodeps -i "$top/RPMS/noarch/uke-policy-fixture-boot-1-1.noarch.rpm"
for release in 1 2; do
 cat > "$top/SPECS/fixture.spec" <<'SPEC'
Name: uke-policy-fixture-updater
Version: 1
Release: @RELEASE@
Summary: Isolated automatic policy fixture
License: MIT
BuildArch: noarch
%description
Only temporary files and a private RPM database.
%install
mkdir -p %{buildroot}@PREFIX@/etc/senemos
printf 'automatic=1\n' > %{buildroot}@PREFIX@/etc/senemos/kernel-update.conf
printf 'automatic=1\nvrr_min_hz=30\n' > %{buildroot}@PREFIX@/etc/senemos/userdata-update.conf
%files
%config(noreplace) @PREFIX@/etc/senemos/kernel-update.conf
%config(noreplace) @PREFIX@/etc/senemos/userdata-update.conf
SPEC
 sed -i "s|@PREFIX@|$prefix|g; s|@RELEASE@|$release|g" "$top/SPECS/fixture.spec"
 if [[ $release == 2 ]]; then
  cat >> "$top/SPECS/fixture.spec" <<SPEC
%post
$helper --policy-migrate-fixture $config $state || exit 1
printf 'migrate\n' >> $work/events
%transfiletriggerin -- $prefix/usr/lib/senemos/kernel-updates
$helper --policy-enabled-fixture $config $state || exit 1
printf 'auto\n' >> $work/events
SPEC
 fi
 rpmbuild -bb --quiet --define "_topdir $top" --define "_rpmdir $top/RPMS" --define "_srcrpmdir $top/SRPMS" --define "_builddir $top/BUILD" --define "_buildrootdir $top/BUILDROOT" --define '_rpmfilename %{ARCH}/%{NAME}-%{VERSION}-%{RELEASE}.%{ARCH}.rpm' --define 'debug_package %{nil}' "$top/SPECS/fixture.spec"
 rpm --dbpath "$work/rpmdb" --nosignature --nodeps -U "$top/RPMS/noarch/uke-policy-fixture-updater-1-$release.noarch.rpm"
 if [[ $release == 1 ]]; then
  printf 'automatic=0\n' > "$config/kernel-update.conf"
  printf 'automatic=0\n' > "$config/userdata-update.conf"
 fi
done
diff -u <(printf 'migrate\nauto\n') "$work/events"
diff -u <(printf 'automatic=0\n') "$state/global-policy.before-v1"
diff -u <(printf 'automatic=0\n') "$state/userdata-policy.before-v1"
printf 'Actual RPM upgrade: modified noreplace configs migrated before the final trigger; existing boot package alone triggered activation once\n'
