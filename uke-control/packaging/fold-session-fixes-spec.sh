#!/usr/bin/bash
set -Eeuo pipefail
spec=${1:?Existing214 spec}
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
awk '
 /^Version: 2[.]14[.]0$/ {print "Version: 2.15.0";version++;next}
 /^Release: 1%\{\?dist\}$/ {print "Release: 2%{?dist}";release++;next}
 /^Source36: copr-control-speaker[.]sh$/ {
  print;print "Source37: uke-control-session-fixes.tar.xz"
  print "Source38: copr-control-session-fixes.sh";source++;next
 }
 $0=="%build" {
  print "mkdir -p %{_builddir}/uke-control-session-fixes"
  print "tar -xJf %{SOURCE37} -C %{_builddir}/uke-control-session-fixes"
  print "bash %{_builddir}/uke-control-session-fixes/packaging/apply-session-fixes.sh \"$boot_root\" %{_builddir}/uke-control-brightness"
  prep++
 }
 $0=="%changelog" {
  print;print "* Mon Nov 09 2026 Uke contributors - 2.15.0-2"
  print "- Key acceptance backups by all observed GPT metadata, not entries alone."
  print "- Merge pending brightness curves across unrelated shutdown settings."
  print "- Preserve rollback, runtime ownership and original desktop packages."
  print "";history=1;changelog++;next
 }
 {if(!history)cohort+=gsub(/2[.]14[.]0-1/,"2.15.0-2");print}
 END{if(version!=3||release!=3||source!=1||prep!=1||changelog!=1||cohort!=12)exit 1}
' "$spec" > "$work/spec"
install -m644 "$work/spec" "$spec"
