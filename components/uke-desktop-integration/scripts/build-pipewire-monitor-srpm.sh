#!/usr/bin/env bash
# Rebuild the exact Fedora source package with the bounded Uke label correction.
set -euo pipefail
juo_root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
juo_srpm=$(realpath "${1:?Original Fedora PipeWire SRPM required}")
juo_out=$(realpath -m "${2:?New RPM build directory required}")
juo_expected=76d09bd1c005d8e9bb43f4bdefa9eb0bc2431cc02164105415295a47825b7d44
test "$(sha256sum "$juo_srpm" | cut -d ' ' -f1)" = "$juo_expected"
test ! -e "$juo_out"
mkdir -p "$juo_out"/{SOURCES,SPECS,SRPMS,BUILD,RPMS}
rpm2cpio "$juo_srpm" > "$juo_out/source.cpio"
(cd "$juo_out/SOURCES" && cpio -idm --quiet < "$juo_out/source.cpio")
rm "$juo_out/source.cpio"
awk '
 /^%global baserelease 1$/ { $0="%global baserelease 1.uke1"; release++ }
 /^Source1:.*pipewire.sysusers$/ { print; print "Source2:        pipewire-monitor-name.sh"; source++; next }
 /^## upstreamable patches$/ { print; print "Patch100:       0001-uke-displayport-monitor-name.patch"; patch++; next }
 /^%check$/ { print; print "bash %{SOURCE2} ."; check++; next }
 /^%changelog$/ {
   print
   print "* Sat Oct 24 2026 Senemos Contributors <75160848+MCC45TR@users.noreply.github.com> - 1.6.10-1.uke1"
   print "- Use actual ELD monitor names for the Uke digital playback sink and ports."
   print ""
   next
 }
 { print }
 END { if (release != 1 || source != 1 || patch != 1 || check != 1) exit 1 }
' "$juo_out/SOURCES/pipewire.spec" > "$juo_out/SPECS/pipewire.spec"
rm "$juo_out/SOURCES/pipewire.spec"
cp "$juo_root/patches/pipewire/0001-uke-displayport-monitor-name.patch" "$juo_out/SOURCES/"
cp "$juo_root/tests/pipewire-monitor-name.sh" "$juo_out/SOURCES/"
rpmbuild -bs --define "_topdir $juo_out" --define '_buildhost copr-builder' "$juo_out/SPECS/pipewire.spec"
