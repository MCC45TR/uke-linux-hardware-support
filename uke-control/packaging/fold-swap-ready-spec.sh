#!/usr/bin/bash
set -Eeuo pipefail
[[ $# == 3 ]]
parent=$(realpath "$1");result=$(realpath -m "$2");contract=$(realpath "$3")
[[ ! -e $result && $(jq -er .release "$contract") == 28 ]]
printf '%s  %s\n' "$(jq -er .parent.spec_sha256 "$contract")" "$parent" | sha256sum -c --strict --quiet -
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
cat > "$work/header" <<'SPEC'
# UKE_SWAP_READY28_HEADER_BEGIN
Source85: swap-ready-overlay.tar.xz
Source86: swap-ready-source.json
Provides: uke-control(swap-ready-v1) = 1
# UKE_SWAP_READY28_HEADER_END
SPEC
cat > "$work/prep" <<'SPEC'
# UKE_SWAP_READY28_PREP_BEGIN
(
overlay=%{_builddir}/uke-control-swap-ready
mkdir "$overlay"
tar -xJf %{SOURCE85} -C "$overlay"
(cd "$overlay";sha256sum -c --strict --quiet SHA256SUMS)
control=%{_builddir}/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/work/uke-control/uke-control-2.8.1
bash "$overlay/apply.sh" "$control" "$overlay"
)
# UKE_SWAP_READY28_PREP_END
SPEC
cat > "$work/build" <<'SPEC'
# UKE_SWAP_READY28_BUILD_BEGIN
(
overlay=%{_builddir}/uke-control-swap-ready
control=%{_builddir}/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/work/uke-control/uke-control-2.8.1
if ! bash "$overlay/tests/swap-ready.sh" "$control" > "$overlay/results.txt" 2>&1;then tail -n 100 "$overlay/results.txt";exit 1;fi
cat "$overlay/results.txt"
test "$(grep -c '^PASS: actual swap startup' "$overlay/results.txt")" = 4
)
# UKE_SWAP_READY28_BUILD_END
SPEC
cat > "$work/install" <<'SPEC'
# UKE_SWAP_READY28_INSTALL_BEGIN
overlay=%{_builddir}/uke-control-swap-ready
install -Dm644 "$overlay/uke-swap-ready.service" %{buildroot}/usr/lib/systemd/system/uke-swap-ready.service
install -Dm644 "$overlay/40-uke-swap-ready.conf" %{buildroot}/usr/lib/systemd/system/uke-swap-ready.service.d/40-uke-swap-ready.conf
install -Dm644 "$overlay/40-uke-swap-startup.conf" %{buildroot}/usr/lib/systemd/system/uke-control.service.d/40-uke-swap-startup.conf
install -Dm644 "$overlay/40-uke-swap-before-session.conf" %{buildroot}/usr/lib/systemd/system/systemd-user-sessions.service.d/40-uke-swap-before-session.conf
install -Dm644 "$overlay/results.txt" %{buildroot}%{_docdir}/uke-control/swap-ready28/results.txt
install -Dm644 "$overlay/docs/SWAP-STARTUP.md" %{buildroot}%{_docdir}/uke-control/swap-ready28/SWAP-STARTUP.md
# UKE_SWAP_READY28_INSTALL_END
SPEC
cat > "$work/check" <<'SPEC'
# UKE_SWAP_READY28_CHECK_BEGIN
test "$(grep -c '^PASS: actual swap startup' %{buildroot}%{_docdir}/uke-control/swap-ready28/results.txt)" = 4
cmp %{_builddir}/uke-control-swap-ready/results.txt %{buildroot}%{_docdir}/uke-control/swap-ready28/results.txt
grep -Fxq 'ExecStart=/usr/bin/uke-control swap ready' %{buildroot}/usr/lib/systemd/system/uke-swap-ready.service.d/40-uke-swap-ready.conf
! grep -q '^ExecStart=' %{buildroot}/usr/lib/systemd/system/uke-control.service.d/40-uke-swap-startup.conf
# UKE_SWAP_READY28_CHECK_END
SPEC
cat > "$work/files" <<'SPEC'
# UKE_SWAP_READY28_FILES_BEGIN
%{_docdir}/uke-control/swap-ready28/
/usr/lib/systemd/system/uke-swap-ready.service
/usr/lib/systemd/system/uke-swap-ready.service.d/40-uke-swap-ready.conf
/usr/lib/systemd/system/uke-control.service.d/40-uke-swap-startup.conf
/usr/lib/systemd/system/systemd-user-sessions.service.d/40-uke-swap-before-session.conf
# UKE_SWAP_READY28_FILES_END
SPEC
awk -v base="$work" '
 function inject(name,path,line){path=base"/"name;while((getline line<path)>0)print line;close(path)}
 /^%changelog$/{history=1;print;print "# UKE_SWAP_READY28_HISTORY_BEGIN";print "* Tue Nov 17 2026 Uke contributors - 2.17.0-28";print "- Reuse current-boot full swap admission before user sessions; preserve changed-state fallback.";print "";print "# UKE_SWAP_READY28_HISTORY_END";next}
 !history{
  if($0=="Release: 27%{?dist}"){$0="Release: 28%{?dist}";releases++}
  versions+=gsub(/2[.]17[.]0-27/,"2.17.0-28")
  if($0=="%description"&&!header++){inject("header")}
  if($0=="%build"){inject("prep");prep++;print;inject("build");build++;next}
  if($0=="install -Dm755 build/uke-control %{buildroot}/usr/bin/uke-control"){print;inject("install");installs++;next}
  if($0=="%check"){print;inject("check");checks++;next}
  if($0=="%files"){print;inject("files");files++;next}
 }
 {print}
 END{if(releases!=3||versions!=12||header<1||prep!=1||build!=1||installs!=1||checks!=1||files!=1)exit 1}
' "$parent" > "$result"
awk '
 /^# UKE_SWAP_READY28_[A-Z]+_BEGIN$/{skip=1;next}
 /^# UKE_SWAP_READY28_[A-Z]+_END$/{skip=0;next}
 skip{next}
 /^%changelog$/{history=1}
 !history{if($0=="Release: 28%{?dist}")$0="Release: 27%{?dist}";gsub(/2[.]17[.]0-28/,"2.17.0-27")}
 {print}
' "$result" | cmp - "$parent"
echo 'Control28 whole-recipe inverse retains every signed Control27 input and prior check.'
