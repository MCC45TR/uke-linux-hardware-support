#!/usr/bin/bash
# Preserve the full Control2.10 cohort and its existing three output names.
set -Eeuo pipefail
spec=$(realpath "${1:?Retained Control spec}")
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
[[ $(grep -Fxc 'Version: 2.10.0' "$spec") == 3 ]]
[[ $(grep -Fxc 'Release: 1%{?dist}' "$spec") == 3 ]]
! grep -q UKE_CONTROL_BOOT_HEALTH_REPORT "$spec"
awk '
 /^Version: 2.10.0$/ {print "Version: 2.11.0";next}
 $0=="%description" {
  print "# UKE_CONTROL_BOOT_HEALTH_REPORT"
  print "Source24: uke-control-boot-health.tar.xz"
  print "Source25: fold-health-spec.sh"
  print "Source26: copr-control-health.sh"
  print "BuildRequires: git-core"
  print "Provides: uke-control(kernel-health-report-v1) = 1"
  print;header++;next
 }
 $0=="%build" {
  print "mkdir -p %{_builddir}/uke-control-boot-health"
  print "tar -xJf %{SOURCE24} -C %{_builddir}/uke-control-boot-health"
  print "bash %{_builddir}/uke-control-boot-health/apply-boot-health-report.sh \"$boot_root\""
  print;print "bash %{_builddir}/uke-boot-integration-1.8.102/work/uke-boot-integration/uke-boot-integration-1.8.102/tests/boot-health-report.sh"
  build++;next
 }
 $0=="%check" {
  print "install -Dm644 %{_builddir}/uke-control-boot-health/BOOT-HEALTH-OBSERVATION.md %{buildroot}%{_docdir}/uke-control/BOOT-HEALTH-OBSERVATION.md"
  print;check++;next
 }
 $0=="%files" {
  print;print "%{_docdir}/uke-control/BOOT-HEALTH-OBSERVATION.md"
  files++;next
 }
 $0=="%changelog" {
  print;print "* Sun Nov 08 2026 Uke contributors - 2.11.0-1"
  print "- Observe unapproved boots in every rollback mode and record the failed health check."
  print "- Keep off-mode approval, slot selection and reboot disabled."
  print "";history=1;changelog++;next
 }
 {if(!history)gsub(/2[.]10[.]0-1/,"2.11.0-1");print}
 END{if(header!=1||build!=1||check!=1||files!=1||changelog!=1)exit 1}
' "$spec" > "$work/spec"
install -m644 "$work/spec" "$spec"
printf 'Control2.11 health-report composition prepared; same runtime/TUI/SDK families and retained lifecycle.\n'
