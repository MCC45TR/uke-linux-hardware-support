#!/usr/bin/env bash
# Rebuild the original Fedora package with the bounded SoC classifier fix.
set -Eeuo pipefail
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
output=$(realpath -m "${1:?Pass the source RPM output directory}")
stage=$(mktemp -d)
trap 'find "$stage" -depth -delete' EXIT
curl --fail --silent --show-error --location --proto '=https' --proto-redir '=https' \
    https://kojipkgs.fedoraproject.org/packages/inxi/3.3.41/2.fc45/src/inxi-3.3.41-2.fc45.src.rpm \
    -o "$stage/parent.src.rpm"
printf '%s  %s\n' ab55de4bab3a64564a6d2c5db2f2fc94bbf704e6f3a89259b1c99f4548788f8e "$stage/parent.src.rpm" | sha256sum -c -
mkdir -p "$stage/rpmbuild"/{SOURCES,SPECS,SRPMS} "$output"
rpm2cpio "$stage/parent.src.rpm" > "$stage/parent.cpio"
cpio -t --quiet < "$stage/parent.cpio" > "$stage/entries"
awk '/^\/|(^|\/)\.\.($|\/)/ {bad=1} END {exit bad}' "$stage/entries"
(cd "$stage/rpmbuild/SOURCES"; cpio -idm --quiet --no-absolute-filenames < "$stage/parent.cpio")
spec=$stage/rpmbuild/SPECS/inxi.spec
awk '
 /^Release:/ {$0="Release:        3%{?dist}"}
 /^%autosetup / {$0=$0 " -p1"}
 /^%install$/ {print "%check\nbash %{SOURCE1} inxi\n"}
 {print}
 /^Source0:/ {print "Source1:        inxi-soc-thermal.sh\nPatch0:         0002-inxi-soc-thermal-classification.patch"}
 /^%changelog$/ {print "* Sun Nov 01 2026 MCC45TR <75160848+MCC45TR@users.noreply.github.com> - 3.3.41-3\n- Classify SoC thermal consumers before display and network hardware\n"}
' "$stage/rpmbuild/SOURCES/inxi.spec" > "$spec"
install -m644 "$owner/patches/0002-inxi-soc-thermal-classification.patch" \
    "$owner/tests/inxi-soc-thermal.sh" "$stage/rpmbuild/SOURCES/"
[[ $(rpmspec -q --srpm --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$spec") == inxi-3.3.41-3.* ]]
rpmbuild -bs --nodeps --define "_topdir $stage/rpmbuild" --define '_buildhost copr-builder' "$spec"
install -m644 "$stage/rpmbuild/SRPMS/"*.src.rpm "$output/"
