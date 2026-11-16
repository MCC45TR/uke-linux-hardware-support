#!/usr/bin/bash
# Preserve the complete signed Control23 recipe and its 77 source inputs.
set -Eeuo pipefail
[[ $# == 3 ]]
parent=$(realpath "$1");result=$(realpath -m "$2");contract=$(realpath "$3")
[[ ! -e $result && ! -L $result && $(jq -er .release "$contract") == 24 ]]
printf '%s  %s\n' "$(jq -er .parent.spec_sha256 "$contract")" "$parent" | sha256sum -c --strict --quiet -
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
cat > "$work/header" <<'SPEC'
# UKE_SUPER_BOOT24_HEADER_BEGIN
Source77: super-boot-overlay.tar.xz
Source78: super-boot-source.json
Provides: uke-control(super-root-boot-v1) = 1
# UKE_SUPER_BOOT24_HEADER_END
SPEC
cat > "$work/prep" <<'SPEC'
# UKE_SUPER_BOOT24_PREP_BEGIN
(
overlay=%{_builddir}/uke-control-super-boot
mkdir "$overlay"
tar -xJf %{SOURCE77} -C "$overlay"
(cd "$overlay";sha256sum -c --strict --quiet SHA256SUMS)
boot=%{_builddir}/uke-boot-integration-1.8.102/work/uke-boot-integration/uke-boot-integration-1.8.102
bash "$overlay/apply.sh" "$boot" "$overlay"
)
# UKE_SUPER_BOOT24_PREP_END
SPEC
cat > "$work/build" <<'SPEC'
# UKE_SUPER_BOOT24_BUILD_BEGIN
(
overlay=%{_builddir}/uke-control-super-boot
boot=%{_builddir}/uke-boot-integration-1.8.102/work/uke-boot-integration/uke-boot-integration-1.8.102
if ! bash "$boot/tests/super-boot-update.sh" "$boot" "$overlay" > "$overlay/native-results.txt" 2>&1;then tail -n 100 "$overlay/native-results.txt";exit 1;fi
cat "$overlay/native-results.txt"
test "$(grep -c '^PASS: actual super boot updater' "$overlay/native-results.txt")" = 4
)
# UKE_SUPER_BOOT24_BUILD_END
SPEC
cat > "$work/install" <<'SPEC'
# UKE_SUPER_BOOT24_INSTALL_BEGIN
overlay=%{_builddir}/uke-control-super-boot
install -Dm644 "$overlay/docs/SUPER-ROOT-BOOT.md" %{buildroot}%{_docdir}/uke-control/SUPER-ROOT-BOOT.md
install -m644 "$overlay/native-results.txt" %{buildroot}%{_docdir}/uke-control/super-boot-native-results.txt
# UKE_SUPER_BOOT24_INSTALL_END
SPEC
cat > "$work/check" <<'SPEC'
# UKE_SUPER_BOOT24_CHECK_BEGIN
(
test "$(id -u)" != 0
set +e
output=$(%{buildroot}/usr/sbin/uke-kernel-update derive-slot-image /missing /missing a 2>&1)
status=$?
set -e
test "$status" = 1
printf '%s\n' "$output" | grep -Fx 'uke-kernel-update: Root privileges are required'
cmp %{_builddir}/uke-control-super-boot/native-results.txt %{buildroot}%{_docdir}/uke-control/super-boot-native-results.txt
)
# UKE_SUPER_BOOT24_CHECK_END
SPEC
awk -v base="$work" '
 function inject(name,path,line){path=base"/"name;while((getline line<path)>0)print line;close(path)}
 /^%changelog$/{history=1;print;print "# UKE_SUPER_BOOT24_HISTORY_BEGIN";print "* Mon Nov 16 2026 Uke contributors - 2.17.0-24";print "- Preserve capability-gated seed4 profiles and destination slot identity in the existing updater.";print "";print "# UKE_SUPER_BOOT24_HISTORY_END";next}
 !history{
  if($0=="Release: 23%{?dist}"){$0="Release: 24%{?dist}";releases++}
  versions+=gsub(/2[.]17[.]0-23/,"2.17.0-24")
  if($0=="%description"&&!header++){inject("header")}
  if($0=="%build"){inject("prep");prep++;print;inject("build");build++;next}
  if($0=="install -Dm755 build/uke-control %{buildroot}/usr/bin/uke-control"){print;inject("install");installs++;next}
  if($0=="%check"){print;inject("check");checks++;next}
  if($0=="%files"){print;print "# UKE_SUPER_BOOT24_FILES_BEGIN";print "%{_docdir}/uke-control/SUPER-ROOT-BOOT.md";print "%{_docdir}/uke-control/super-boot-native-results.txt";print "# UKE_SUPER_BOOT24_FILES_END";files++;next}
 }
 {print}
 END{if(releases!=3||versions!=12||header<1||prep!=1||build!=1||installs!=1||checks!=1||files!=1)exit 1}
' "$parent" > "$result"
awk '
 /^# UKE_SUPER_BOOT24_[A-Z]+_BEGIN$/{skip=1;next}
 /^# UKE_SUPER_BOOT24_[A-Z]+_END$/{skip=0;next}
 skip{next}
 /^%changelog$/{history=1}
 !history{if($0=="Release: 24%{?dist}")$0="Release: 23%{?dist}";gsub(/2[.]17[.]0-24/,"2.17.0-23")}
 {print}
' "$result" | cmp - "$parent"
echo 'Control24 whole-recipe inverse retains every Control23 source, check, service, policy and existing runtime owner.'
