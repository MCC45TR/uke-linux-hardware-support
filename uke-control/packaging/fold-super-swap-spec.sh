#!/usr/bin/bash
# Preserve the complete signed Control24 recipe and all 79 source inputs.
set -Eeuo pipefail
[[ $# == 3 ]]
parent=$(realpath "$1");result=$(realpath -m "$2");contract=$(realpath "$3")
[[ ! -e $result && ! -L $result && $(jq -er .release "$contract") == 25 ]]
printf '%s  %s\n' "$(jq -er .parent.spec_sha256 "$contract")" "$parent" | sha256sum -c --strict --quiet -
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
cat > "$work/header" <<'SPEC'
# UKE_SUPER_SWAP25_HEADER_BEGIN
Source79: super-swap-overlay.tar.xz
Source80: super-swap-source.json
Provides: uke-control(logical-swap-v1) = 1
Provides: uke-control(setup-cdc-reboot-v1) = 1
Requires: android-tools device-mapper
# UKE_SUPER_SWAP25_HEADER_END
SPEC
cat > "$work/prep" <<'SPEC'
# UKE_SUPER_SWAP25_PREP_BEGIN
(
overlay=%{_builddir}/uke-control-super-swap
mkdir "$overlay"
tar -xJf %{SOURCE79} -C "$overlay"
(cd "$overlay";sha256sum -c --strict --quiet SHA256SUMS)
control=%{_builddir}/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/work/uke-control/uke-control-2.8.1
bash "$overlay/apply.sh" "$control" "$overlay"
)
# UKE_SUPER_SWAP25_PREP_END
SPEC
cat > "$work/build" <<'SPEC'
# UKE_SUPER_SWAP25_BUILD_BEGIN
(
overlay=%{_builddir}/uke-control-super-swap
control=%{_builddir}/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/work/uke-control/uke-control-2.8.1
for check in super-swap setup-cdc;do
 if ! bash "$control/tests/$check.sh" "$control" > "$overlay/$check-results.txt" 2>&1;then tail -n 100 "$overlay/$check-results.txt";exit 1;fi
 cat "$overlay/$check-results.txt"
 test "$(grep -c '^PASS: actual ' "$overlay/$check-results.txt")" = 4
done
)
# UKE_SUPER_SWAP25_BUILD_END
SPEC
cat > "$work/install" <<'SPEC'
# UKE_SUPER_SWAP25_INSTALL_BEGIN
overlay=%{_builddir}/uke-control-super-swap
install -Dm755 "$overlay/prepare.sh" %{buildroot}/usr/libexec/uke-control/super-swap/prepare.sh
for file in "$overlay/runtime/"*.sh;do install -m755 "$file" %{buildroot}/usr/libexec/uke-control/super-swap/;done
install -Dm644 "$overlay/docs/LOGICAL-SWAP.md" %{buildroot}%{_docdir}/uke-control/LOGICAL-SWAP.md
install -m644 "$overlay/super-swap-results.txt" "$overlay/setup-cdc-results.txt" %{buildroot}%{_docdir}/uke-control/
# UKE_SUPER_SWAP25_INSTALL_END
SPEC
cat > "$work/check" <<'SPEC'
# UKE_SUPER_SWAP25_CHECK_BEGIN
(
test "$(id -u)" != 0
set +e
output=$(%{buildroot}/usr/libexec/uke-control/super-swap/prepare.sh --prepare 2>&1)
status=$?
set -e
test "$status" = 1
printf '%s\n' "$output" | grep -Fx 'UKE_SUPER_SWAP_REJECTED: root required'
cmp %{_builddir}/uke-control-super-swap/super-swap-results.txt %{buildroot}%{_docdir}/uke-control/super-swap-results.txt
for file in %{_builddir}/uke-control-super-swap/runtime/*.sh;do cmp "$file" "%{buildroot}/usr/libexec/uke-control/super-swap/${file##*/}";done
)
# UKE_SUPER_SWAP25_CHECK_END
SPEC
awk -v base="$work" '
 function inject(name,path,line){path=base"/"name;while((getline line<path)>0)print line;close(path)}
 /^%changelog$/{history=1;print;print "# UKE_SUPER_SWAP25_HISTORY_BEGIN";print "* Mon Nov 16 2026 Uke contributors - 2.17.0-25";print "- Admit the qualified logical swap before file fallback; use the installed systemctl for setup USB reboot.";print "";print "# UKE_SUPER_SWAP25_HISTORY_END";next}
 !history{
  if($0=="Release: 24%{?dist}"){$0="Release: 25%{?dist}";releases++}
  versions+=gsub(/2[.]17[.]0-24/,"2.17.0-25")
  if($0=="# UKE_SETUP_CDC23_BUILD_BEGIN")legacy=1
  if($0=="# UKE_SETUP_CDC23_BUILD_END")legacy=0
  if(legacy&&$0~/^control=/){$0="control=%{_builddir}/uke-control-super-swap/legacy";legacy_views++}
  if($0=="%description"&&!header++){inject("header")}
  if($0=="%build"){inject("prep");prep++;print;inject("build");build++;next}
  if($0=="install -Dm755 build/uke-control %{buildroot}/usr/bin/uke-control"){print;inject("install");installs++;next}
  if($0=="%check"){print;inject("check");checks++;next}
  if($0=="%files"){print;print "# UKE_SUPER_SWAP25_FILES_BEGIN";print "/usr/libexec/uke-control/super-swap/";print "%{_docdir}/uke-control/LOGICAL-SWAP.md";print "%{_docdir}/uke-control/super-swap-results.txt";print "%{_docdir}/uke-control/setup-cdc-results.txt";print "# UKE_SUPER_SWAP25_FILES_END";files++;next}
 }
 {print}
 END{if(releases!=3||versions!=12||header<1||prep!=1||build!=1||installs!=1||checks!=1||files!=1||legacy_views!=1)exit 1}
' "$parent" > "$result"
awk '
 /^# UKE_SUPER_SWAP25_[A-Z]+_BEGIN$/{skip=1;next}
 /^# UKE_SUPER_SWAP25_[A-Z]+_END$/{skip=0;next}
 skip{next}
 /^%changelog$/{history=1}
 !history{
  if($0=="Release: 25%{?dist}")$0="Release: 24%{?dist}";gsub(/2[.]17[.]0-25/,"2.17.0-24")
  if($0=="control=%{_builddir}/uke-control-super-swap/legacy")$0="control=%{_builddir}/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/work/uke-control/uke-control-2.8.1"
 }
 {print}
' "$result" | cmp - "$parent"
echo 'Control25 whole-recipe inverse preserves all79 inputs, checks and runtime owners; retained CDC proof runs on its exact prepared view.'
