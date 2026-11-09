#!/usr/bin/bash
# Compose only over the exact admitted Control 2.13 cohort.
set -Eeuo pipefail
spec=$(realpath "${1:?Control2.13 spec}")
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
[[ $(sha256sum "$spec" | cut -d' ' -f1) == 036d9d7077ea5f3c39faff9f078e721d7c17ed9a2c04752ab6c447802ab31b6a ]]
[[ $(grep -Fxc 'Version: 2.13.0' "$spec") == 3 ]]
! grep -q UKE_CONTROL_SPEAKER_RUNTIME "$spec"
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
awk -v migration="$owner/packaging/speaker-state.sh" '
 function state(phase, line){
  print "(\n:\n# UKE_CONTROL_SPEAKER_"phase"_BEGIN"
  print "phase=" tolower(phase)
  while((getline line < migration)>0){gsub(/%/,"%%",line);print line}
  close(migration)
  print "# UKE_CONTROL_SPEAKER_"phase"_END\n) || exit $?"
 }
 /^Version: 2.13.0$/ {print "Version: 2.14.0";next}
 $0=="License: MIT AND Apache-2.0 AND GPL-3.0-or-later" {
  print $0 " AND GPL-2.0-only";license++;next
 }
 $0=="%description" {
  print "# UKE_CONTROL_SPEAKER_RUNTIME"
  print "Source33: uke-control-speaker-runtime.tar.xz"
  print "Source34: retained-speaker-runtime-0.4-3.tar.gz"
  print "Source35: fold-speaker-spec.sh"
  print "Source36: copr-control-speaker.sh"
  print "Requires: bash curl jq alsa-utils"
  print "Provides: uke-speaker-runtime = 0.4-4%{?dist}"
  print "Provides: uke-speaker-runtime%{?_isa} = 0.4-4%{?dist}"
  print "Provides: uke-speaker-runtime(gain-readiness-v1) = 1"
  print "Provides: uke-control(speaker-runtime-v1) = 1"
  print "Obsoletes: uke-speaker-runtime <= 0.4-3%{?dist}"
  print;header++;next
 }
 $0=="%build" {
  print "mkdir -p %{_builddir}/uke-control-speaker-overlay %{_builddir}/uke-control-speaker"
  print "tar -xJf %{SOURCE33} -C %{_builddir}/uke-control-speaker-overlay"
  print "tar -xzf %{SOURCE34} -C %{_builddir}/uke-control-speaker"
  print "git -C %{_builddir}/uke-control-speaker init -q"
  print "git -C %{_builddir}/uke-control-speaker apply --check --whitespace=error %{_builddir}/uke-control-speaker-overlay/packaging/speaker-runtime.patch"
  print "git -C %{_builddir}/uke-control-speaker apply --whitespace=error %{_builddir}/uke-control-speaker-overlay/packaging/speaker-runtime.patch"
  print "install -m644 %{_builddir}/uke-control-speaker-overlay/packaging/SPEAKER-RUNTIME-GPL-2.0 SPEAKER-RUNTIME-GPL-2.0"
  print "install -m644 %{_builddir}/uke-control-speaker/LICENSE SPEAKER-RUNTIME-ORIGINAL-LICENSE"
  print
  print "bash %{_builddir}/uke-control-speaker-overlay/tests/speaker-ensure-loaded.sh %{_builddir}/uke-control-speaker"
  print "bash %{_builddir}/uke-control-speaker-overlay/tests/speaker-provider.sh %{_builddir}/uke-control-speaker/src/audio/speaker-gain-preflight.sh"
  build++;next
 }
 $0=="%install" {
  print "%{__cxx} %{build_cxxflags} -std=c++20 -Wall -Wextra -Werror %{_builddir}/uke-control-speaker/src/audio/uke-speaker-runtime.cpp %{build_ldflags} -lasound -lcrypto -o %{_builddir}/uke-control-speaker/uke-speaker-runtime"
  print;install++;next
 }
 $0=="%check" {
  print "install -Dm755 %{_builddir}/uke-control-speaker/uke-speaker-runtime %{buildroot}/usr/bin/uke-speaker-runtime"
  print "install -Dm755 %{_builddir}/uke-control-speaker/src/audio/speaker-gain-preflight.sh %{buildroot}/usr/bin/uke-speaker-gain-preflight"
  print "install -Dm755 %{_builddir}/uke-control-speaker/src/audio/prepare-generic-speaker-data.sh %{buildroot}/usr/libexec/uke-speaker-prepare-data"
  print "install -Dm644 %{_builddir}/uke-control-speaker/src/audio/systemd/uke-speaker-provision@.service %{buildroot}/usr/lib/systemd/system/uke-speaker-provision@.service"
  print "install -Dm644 %{_builddir}/uke-control-speaker/src/audio/udev/70-uke-speaker-provision.rules %{buildroot}/usr/lib/udev/rules.d/70-uke-speaker-provision.rules"
  print "install -Dm644 %{_builddir}/uke-control-speaker-overlay/docs/SPEAKER-RUNTIME-CONSOLIDATION.md %{buildroot}%{_docdir}/uke-control/SPEAKER-RUNTIME-CONSOLIDATION.md"
  print
  print "for compiler in g++ clang++;do"
  print " CXX=$compiler bash %{_builddir}/uke-control-speaker/tests/audio-speaker-runtime-contract.sh"
  print " CXX=$compiler bash %{_builddir}/uke-control-speaker/tests/audio-speaker-service-contract.sh"
  print "done"
  print "bash %{_builddir}/uke-control-speaker/tests/audio-speaker-preflight-contract.sh"
  print "bash %{_builddir}/uke-control-speaker-overlay/tests/speaker-state.sh %{_specdir}/uke-control.spec"
  check++;next
 }
 $0=="%pre -n uke-control" {print;state("SAVE");pre++;next}
 $0=="%posttrans -n uke-control" {print;state("RESTORE");posttrans++;next}
 $0=="%preun -n uke-control" {
  print;print "%systemd_preun uke-speaker-provision@.service";preun++;next
 }
 $0=="%files" {
  print
  print "/usr/bin/uke-speaker-runtime"
  print "/usr/bin/uke-speaker-gain-preflight"
  print "/usr/libexec/uke-speaker-prepare-data"
  print "/usr/lib/systemd/system/uke-speaker-provision@.service"
  print "/usr/lib/udev/rules.d/70-uke-speaker-provision.rules"
  print "%{_docdir}/uke-control/SPEAKER-RUNTIME-CONSOLIDATION.md"
  print "%license SPEAKER-RUNTIME-GPL-2.0 SPEAKER-RUNTIME-ORIGINAL-LICENSE"
  files++;next
 }
 $0=="%changelog" {
  print;print "* Mon Nov 09 2026 Uke contributors - 2.14.0-1"
  print "- Fold guarded speaker provisioning into Control with the existing paths and sound-device trigger."
  print "- Restore active services without retransmitting already staged calibration; retain gain223."
  print "";history=1;changelog++;next
 }
 {if(!history)gsub(/2[.]13[.]0-1/,"2.14.0-1");print}
 END{if(license!=1||header!=1||build!=1||install!=1||check!=1||pre!=1||posttrans!=1||preun!=1||files!=1||changelog!=1)exit 1}
' "$spec" > "$work/spec"
install -m644 "$work/spec" "$spec"
