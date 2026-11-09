#!/usr/bin/bash
# Compose one opt-in preference layer over the exact admitted Control cohort.
set -Eeuo pipefail
[[ $# == 1 ]]
spec=$(realpath "${1:?Control2.12.3 spec}")
[[ $(grep -Fxc 'Version: 2.12.3' "$spec") == 3 ]]
if grep -q UKE_CONTROL_BRIGHTNESS_CURVE "$spec";then exit 1;fi
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
awk '
 /^Version: 2.12.3$/ {print "Version: 2.13.0";next}
 $0=="%description" {
  print "# UKE_CONTROL_BRIGHTNESS_CURVE"
  print "Source30: uke-control-brightness-curve.tar.xz"
  print "Source31: fold-brightness-spec.sh"
  print "Source32: copr-control-brightness.sh"
  print "Provides: uke-control(brightness-curve-v1) = 1"
  print;header++;next
 }
 $0=="%build" {
  print "mkdir -p %{_builddir}/uke-control-brightness"
  print "tar -xJf %{SOURCE30} -C %{_builddir}/uke-control-brightness"
  print "mapfile -t curve_main < <(find %{_builddir}/uke-control-2.8.1/work -type f -path \047*/src/main.cpp\047)"
  print "test ${#curve_main[@]} = 1"
  print "curve_control=${curve_main[0]%/src/main.cpp}"
  print "printf \04724337befa46bdf31836441d7430cb6b928131b9ccb26487aa5afbec47180df0f  %s/src/main.cpp\n5675f2d70ab9d56948459c596fe2f564232f0ad178305db71f0461a5e10b402d  %s/src/cli.cpp\n\047 \"$curve_control\" \"$curve_control\" | sha256sum -c --strict --quiet -"
  print "git -C %{_builddir}/uke-control-brightness init -q"
  print "git -C \"$curve_control\" --git-dir %{_builddir}/uke-control-brightness/.git --work-tree \"$curve_control\" apply -p2 --check --whitespace=error %{_builddir}/uke-control-brightness/packaging/brightness-curve-cli.patch"
  print "git -C \"$curve_control\" --git-dir %{_builddir}/uke-control-brightness/.git --work-tree \"$curve_control\" apply -p2 --whitespace=error %{_builddir}/uke-control-brightness/packaging/brightness-curve-cli.patch"
  print;build++;next
 }
 $0=="%install" {
  print "bash %{_builddir}/uke-control-brightness/tests/brightness-curve.sh"
  print "%{__cxx} %{build_cxxflags} -std=c++20 -Wall -Wextra -Werror $(pkg-config --cflags jsoncpp libsystemd) %{_builddir}/uke-control-brightness/src/brightness-curve.cpp %{build_ldflags} $(pkg-config --libs jsoncpp libsystemd) -o %{_builddir}/uke-control-brightness/brightness-curve"
  print;install++;next
 }
 $0=="%check" {
  print "install -Dm755 %{_builddir}/uke-control-brightness/brightness-curve %{buildroot}/usr/libexec/uke-control/brightness-curve"
  print "install -Dm644 %{_builddir}/uke-control-brightness/packaging/30-uke-brightness-curve.sh %{buildroot}/etc/xdg/plasma-workspace/env/30-uke-brightness-curve.sh"
  print "install -Dm644 %{_builddir}/uke-control-brightness/docs/BRIGHTNESS-CURVE.md %{buildroot}%{_docdir}/uke-control/BRIGHTNESS-CURVE.md"
  print "test -x %{buildroot}/usr/libexec/uke-control/brightness-curve"
  print "sh -n %{buildroot}/etc/xdg/plasma-workspace/env/30-uke-brightness-curve.sh"
  print;check++;next
 }
 $0=="%files" {
  print;print "/usr/libexec/uke-control/brightness-curve"
  print "%config(noreplace) /etc/xdg/plasma-workspace/env/30-uke-brightness-curve.sh"
  print "%{_docdir}/uke-control/BRIGHTNESS-CURVE.md";files++;next
 }
 $0=="%changelog" {
  print;print "* Mon Nov 09 2026 Uke contributors - 2.13.0-1"
  print "- Queue a reversible OSS normal brightness curve for a fresh original Plasma session."
  print "- Preserve other output preferences and refuse active or changed configurations."
  print "";history=1;changelog++;next
 }
 {if(!history)gsub(/2[.]12[.]3-1/,"2.13.0-1");print}
 END{if(header!=1||build!=1||install!=1||check!=1||files!=1||changelog!=1)exit 1}
' "$spec" > "$work/spec"
install -m644 "$work/spec" "$spec"
