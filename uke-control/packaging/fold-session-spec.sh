#!/usr/bin/bash
# Text-only composition over the admitted complete Control 2.9.0-3 recipe.
set -Eeuo pipefail
spec=$(realpath "${1:?Retained Control spec}")
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
[[ $(grep -Fxc 'Version: 2.9.0' "$spec") == 3 ]]
[[ $(grep -Fxc 'Release: 3%{?dist}' "$spec") == 3 ]]
if grep -q UKE_CONTROL_SESSION_BRIDGE "$spec";then exit 1;fi
awk '
 /^Version: 2.9.0$/ {print "Version: 2.10.0";next}
 /^Release: 3%\{[?]dist\}$/ {print "Release: 1%{?dist}";next}
 $0=="%description" {
  print "# UKE_CONTROL_SESSION_BRIDGE"
  print "Source21: uke-control-session-bridge.tar.xz"
  print "Source22: copr-control-session.sh"
  print "Source23: fold-session-spec.sh"
  print "BuildRequires: pkgconfig(alsa)"
  print "Provides: uke-speaker-orientation = 0.2.0-3%{?dist}"
  print "Provides: uke-speaker-orientation%{?_isa} = 0.2.0-3%{?dist}"
  print "Provides: uke-control(session-bridge-v1) = 1"
  print "Obsoletes: uke-speaker-orientation <= 0.2.0-2%{?dist}"
  print;header++;next
 }
 $0=="%prep"{print;prep++;next}
 $0=="%build" {
  print "mkdir -p %{_builddir}/uke-control-session"
  print "tar -xJf %{SOURCE21} -C %{_builddir}/uke-control-session"
  print;build++;next
 }
 $0=="%install" {
  print "("
  print "cd %{_builddir}/uke-control-session"
  print "%{__cxx} %{build_cxxflags} -std=c++17 -Wall -Wextra -Werror $(pkg-config --cflags alsa wayland-client libsystemd) src/session/speaker-orientation.cpp %{build_ldflags} $(pkg-config --libs alsa wayland-client libsystemd) -o uke-speaker-orientation"
  print ")"
  print;install++;next
 }
 $0=="%check" {
  print "install -Dm755 %{_builddir}/uke-control-session/uke-speaker-orientation %{buildroot}/usr/libexec/uke-speaker-orientation"
  print "install -Dm644 %{_builddir}/uke-control-session/packaging/uke-speaker-orientation.desktop %{buildroot}/etc/xdg/autostart/uke-speaker-orientation.desktop"
  print "install -Dm644 %{_builddir}/uke-control-session/docs/SPEAKER-ORIENTATION.md %{buildroot}%{_docdir}/uke-control/SPEAKER-ORIENTATION.md"
  print;print "bash %{_builddir}/uke-control-session/tests/session/speaker-orientation.sh"
  print "test -x %{buildroot}/usr/libexec/uke-speaker-orientation"
  check++;next
 }
 $0=="%files" {
  print;print "/usr/libexec/uke-speaker-orientation"
  print "%config(noreplace) /etc/xdg/autostart/uke-speaker-orientation.desktop"
  print "%{_docdir}/uke-control/SPEAKER-ORIENTATION.md"
  files++;next
 }
 $0=="%changelog" {
  print;print "* Sun Nov 08 2026 Uke contributors - 2.10.0-1"
  print "- Fold the event-driven speaker session bridge into Control."
  print "- Retain the existing executable/autostart paths, settings and kernel compatibility."
  print "";changelog++;history=1;next
 }
 {if(!history)gsub(/2[.]9[.]0-3/,"2.10.0-1");print}
 END{if(header!=1||prep!=1||build!=1||install!=1||check!=1||files!=1||changelog!=1)exit 1}
' "$spec" > "$work/folded.spec"
install -m644 "$work/folded.spec" "$spec"
printf 'Control session composition: one existing Control runtime, exact retained lifecycle, old autostart/config paths and mandatory matching TUI.\n'
