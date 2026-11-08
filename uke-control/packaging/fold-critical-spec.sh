#!/usr/bin/bash
set -Eeuo pipefail
spec=$(realpath "${1:?Control2.11 spec}")
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
[[ $(grep -Fxc 'Version: 2.11.0' "$spec") == 3 ]]
! grep -q UKE_CONTROL_CRITICAL_ROLLBACK "$spec"
awk '
 /^Version: 2.11.0$/ {print "Version: 2.12.2";next}
 $0=="%description" {
  print "# UKE_CONTROL_CRITICAL_ROLLBACK"
  print "Source27: uke-control-critical-rollback.tar.xz"
  print "Source28: fold-critical-spec.sh"
  print "Source29: copr-control-critical.sh"
  print "Provides: uke-control(critical-rollback-v1) = 1"
  print;header++;next
 }
 $0=="%build" {
  print "mkdir -p %{_builddir}/uke-control-critical-rollback"
  print "tar -xJf %{SOURCE27} -C %{_builddir}/uke-control-critical-rollback"
  print "mapfile -t critical_status < <(find %{_builddir}/uke-control-2.8.1/work -type f -path \047*/src/update-status.cpp\047)"
  print "test ${#critical_status[@]} = 1"
  print "bash %{_builddir}/uke-control-critical-rollback/apply-critical-rollback.sh \"$boot_root\" \"${critical_status[0]}\""
  print
  print "bash %{_builddir}/uke-boot-integration-1.8.102/work/uke-boot-integration/uke-boot-integration-1.8.102/tests/critical-health.sh"
  build++;next
 }
 $0=="%check" {
  print "install -Dm644 %{_builddir}/uke-control-critical-rollback/CRITICAL-ROLLBACK.md %{buildroot}%{_docdir}/uke-control/CRITICAL-ROLLBACK.md"
  print;check++;next
 }
 $0=="%files" {print;print "%{_docdir}/uke-control/CRITICAL-ROLLBACK.md";files++;next}
 $0=="%changelog" {
  print;print "* Sun Nov 08 2026 Uke contributors - 2.12.2-1"
  print "- Distinguish confirmed ADSP crashes from ordinary boot-health failures."
  print "- Permit bound critical-only fallback in permissive mode with verified rollback."
  print "";history=1;changelog++;next
 }
 {if(!history)gsub(/2[.]11[.]0-1/,"2.12.2-1");print}
 END{if(header!=1||build!=1||check!=1||files!=1||changelog!=1)exit 1}
' "$spec" > "$work/spec"
install -m644 "$work/spec" "$spec"
