#!/usr/bin/bash
# Preserve every signed Control25 source, recipe phase and runtime owner.
set -Eeuo pipefail
[[ $# == 3 ]]
parent=$(realpath "$1");result=$(realpath -m "$2");contract=$(realpath "$3")
[[ ! -e $result && $(jq -er .release "$contract") == 26 ]]
printf '%s  %s\n' "$(jq -er .parent.spec_sha256 "$contract")" "$parent" | sha256sum -c --strict --quiet -
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
cat > "$work/header" <<'SPEC'
# UKE_BOOT_STARTUP26_HEADER_BEGIN
Source81: boot-startup-overlay.tar.xz
Source82: boot-startup-source.json
Provides: uke-control(boot-serialization-v1) = 1
# UKE_BOOT_STARTUP26_HEADER_END
SPEC
cat > "$work/prep" <<'SPEC'
# UKE_BOOT_STARTUP26_PREP_BEGIN
(
overlay=%{_builddir}/uke-control-boot-startup
mkdir "$overlay"
tar -xJf %{SOURCE81} -C "$overlay"
(cd "$overlay";sha256sum -c --strict --quiet SHA256SUMS)
control=%{_builddir}/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/work/uke-control/uke-control-2.8.1
bash "$overlay/apply.sh" "$control" "$overlay"
)
# UKE_BOOT_STARTUP26_PREP_END
SPEC
cat > "$work/build" <<'SPEC'
# UKE_BOOT_STARTUP26_BUILD_BEGIN
(
overlay=%{_builddir}/uke-control-boot-startup
control=%{_builddir}/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/work/uke-control/uke-control-2.8.1
for check in boot-lock setup-cdc super-swap;do
 if ! bash "$control/tests/$check.sh" "$control" > "$overlay/$check-results.txt" 2>&1;then tail -n 100 "$overlay/$check-results.txt";exit 1;fi
 cat "$overlay/$check-results.txt"
 test "$(grep -c '^PASS: actual ' "$overlay/$check-results.txt")" = 4
done
)
# UKE_BOOT_STARTUP26_BUILD_END
SPEC
cat > "$work/install" <<'SPEC'
# UKE_BOOT_STARTUP26_INSTALL_BEGIN
overlay=%{_builddir}/uke-control-boot-startup
for name in boot-lock setup-cdc super-swap;do
 install -Dm644 "$overlay/$name-results.txt" %{buildroot}%{_docdir}/uke-control/startup26/$name-results.txt
done
# UKE_BOOT_STARTUP26_INSTALL_END
SPEC
cat > "$work/clock" <<'SPEC'
# UKE_BOOT_STARTUP26_CLOCK_BEGIN
install -m644 %{_builddir}/uke-control-boot-startup/75-uke-clock-floor.conf %{buildroot}/usr/lib/systemd/system/chronyd.service.d/75-uke-clock-floor.conf
# UKE_BOOT_STARTUP26_CLOCK_END
SPEC
cat > "$work/check" <<'SPEC'
# UKE_BOOT_STARTUP26_CHECK_BEGIN
cmp %{_builddir}/uke-control-boot-startup/75-uke-clock-floor.conf %{buildroot}/usr/lib/systemd/system/chronyd.service.d/75-uke-clock-floor.conf
grep -Fx 'StateDirectory=senemos/installation' %{buildroot}/usr/lib/systemd/system/chronyd.service.d/75-uke-clock-floor.conf
grep -Fx 'StateDirectoryMode=0700' %{buildroot}/usr/lib/systemd/system/chronyd.service.d/75-uke-clock-floor.conf
for name in boot-lock setup-cdc super-swap;do
 cmp %{_builddir}/uke-control-boot-startup/$name-results.txt %{buildroot}%{_docdir}/uke-control/startup26/$name-results.txt
done
# UKE_BOOT_STARTUP26_CHECK_END
SPEC
awk -v base="$work" '
 function inject(name,path,line){path=base"/"name;while((getline line<path)>0)print line;close(path)}
 /^%changelog$/{history=1;print;print "# UKE_BOOT_STARTUP26_HISTORY_BEGIN";print "* Mon Nov 16 2026 Uke contributors - 2.17.0-26";print "- Serialize boot swap and setup USB operations; prepare the clock floor before sandboxing.";print "";print "# UKE_BOOT_STARTUP26_HISTORY_END";next}
 !history{
  if($0=="Release: 25%{?dist}"){$0="Release: 26%{?dist}";releases++}
  versions+=gsub(/2[.]17[.]0-25/,"2.17.0-26")
  if($0=="# UKE_SUPER_SWAP25_BUILD_BEGIN")legacy=1
  if($0=="# UKE_SUPER_SWAP25_BUILD_END")legacy=0
  if(legacy&&$0~/^control=/){$0="control=%{_builddir}/uke-control-boot-startup/legacy";legacy_views++}
  if($0=="%description"&&!header++){inject("header")}
  if($0=="%build"){inject("prep");prep++;print;inject("build");build++;next}
  if($0=="install -Dm755 build/uke-control %{buildroot}/usr/bin/uke-control"){print;inject("install");installs++;next}
  if($0=="install -Dm644 src/units/75-uke-clock-floor.conf %{buildroot}/usr/lib/systemd/system/chronyd.service.d/75-uke-clock-floor.conf"){print;inject("clock");clock++;next}
  if($0=="%check"){print;inject("check");checks++;next}
  if($0=="%files"){print;print "# UKE_BOOT_STARTUP26_FILES_BEGIN";print "%{_docdir}/uke-control/startup26/";print "# UKE_BOOT_STARTUP26_FILES_END";files++;next}
 }
 {print}
 END{if(releases!=3||versions!=12||header<1||prep!=1||build!=1||installs!=1||clock!=1||checks!=1||files!=1||legacy_views!=1)exit 1}
' "$parent" > "$result"
awk '
 /^# UKE_BOOT_STARTUP26_[A-Z]+_BEGIN$/{skip=1;next}
 /^# UKE_BOOT_STARTUP26_[A-Z]+_END$/{skip=0;next}
 skip{next}
 /^%changelog$/{history=1}
 !history{
  if($0=="Release: 26%{?dist}")$0="Release: 25%{?dist}";gsub(/2[.]17[.]0-26/,"2.17.0-25")
  if($0=="control=%{_builddir}/uke-control-boot-startup/legacy")$0="control=%{_builddir}/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/work/uke-control/uke-control-2.8.1"
 }
 {print}
' "$result" | cmp - "$parent"
echo 'Control26 whole-recipe inverse retains all81 parent inputs and existing package names; previous native checks use their exact prepared view.'
