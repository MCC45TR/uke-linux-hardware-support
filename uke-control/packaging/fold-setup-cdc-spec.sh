#!/usr/bin/bash
# Preserve the complete signed Control22 recipe, sources and existing ownership.
set -Eeuo pipefail
[[ $# == 3 ]]
parent=$(realpath "$1");result=$(realpath -m "$2");contract=$(realpath "$3")
[[ ! -e $result && ! -L $result && $(jq -er .release "$contract") == 23 ]]
printf '%s  %s\n' "$(jq -er .parent.spec_sha256 "$contract")" "$parent" | sha256sum -c --strict --quiet -
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
cat > "$work/header" <<'SPEC'
# UKE_SETUP_CDC23_HEADER_BEGIN
Source74: setup-cdc-overlay.tar.xz
Source75: setup-cdc-native-proof.rpm
Source76: setup-cdc-source.json
Provides: uke-control(setup-cdc-v1) = 1
# UKE_SETUP_CDC23_HEADER_END
SPEC
cat > "$work/prep" <<'SPEC'
# UKE_SETUP_CDC23_PREP_BEGIN
(
overlay=%{_builddir}/uke-control-setup-cdc
mkdir "$overlay"
tar -xJf %{SOURCE74} -C "$overlay"
(cd "$overlay";sha256sum -c --strict --quiet SHA256SUMS)
printf '%s  %s\n' "$(jq -er .native_proof.sha256 %{SOURCE76})" %{SOURCE75} | sha256sum -c --strict --quiet -
mkdir "$overlay/proof"
rpm2cpio %{SOURCE75} | (cd "$overlay/proof";cpio -idm --quiet --no-absolute-filenames --no-preserve-owner)
proof="$overlay/proof/usr/share/uke-control-setup-source-check"
test "$(grep -c '^PASS: actual setup policy' "$proof/native-results.txt")" = 4
(cd "$overlay";awk '$2~/^(src\/setup-cdc\.(cpp|hpp)|tests\/setup-cdc\.(cpp|sh))$/' "$proof/qualified-source.sha256" > qualified-new-source.sha256;test "$(wc -l < qualified-new-source.sha256)" = 4;sha256sum -c --strict --quiet qualified-new-source.sha256)
control=%{_builddir}/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/work/uke-control/uke-control-2.8.1
bash "$overlay/packaging/apply-setup-cdc.sh" "$control" "$overlay"
)
# UKE_SETUP_CDC23_PREP_END
SPEC
cat > "$work/build" <<'SPEC'
# UKE_SETUP_CDC23_BUILD_BEGIN
(
control=%{_builddir}/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/work/uke-control/uke-control-2.8.1
overlay=%{_builddir}/uke-control-setup-cdc
(cd "$control";sha256sum -c --strict --quiet "$overlay/qualified-new-source.sha256")
if ! bash "$control/tests/setup-cdc.sh" "$control" > "$overlay/prepared-native-results.txt" 2>&1;then tail -n 80 "$overlay/prepared-native-results.txt";exit 1;fi
cat "$overlay/prepared-native-results.txt"
test "$(grep -c '^PASS: actual setup policy' "$overlay/prepared-native-results.txt")" = 4
)
# UKE_SETUP_CDC23_BUILD_END
SPEC
cat > "$work/install" <<'SPEC'
# UKE_SETUP_CDC23_INSTALL_BEGIN
setup=%{_builddir}/uke-control-setup-cdc/packaging
for unit in uke-control-setup-cdc.service uke-control-setup-console.service uke-control-setup-cdc-finish.service uke-control-setup-cdc-finish.path;do
 install -Dm644 "$setup/$unit" "%{buildroot}/usr/lib/systemd/system/$unit"
done
install -Dm644 "$setup/91-uke-control-setup-cdc.rules" %{buildroot}/usr/lib/udev/rules.d/91-uke-control-setup-cdc.rules
install -Dm644 "$setup/81-uke-control-setup-cdc.preset" %{buildroot}/usr/lib/systemd/system-preset/81-uke-control-setup-cdc.preset
# Activation is reserved for an explicitly marked fresh Plasma image.
test ! -e %{buildroot}/etc/uke-control/first-setup-cdc
# UKE_SETUP_CDC23_INSTALL_END
SPEC
cat > "$work/check" <<'SPEC'
# UKE_SETUP_CDC23_CHECK_BEGIN
(
test "$(id -u)" != 0
set +e
output=$(LD_LIBRARY_PATH=%{buildroot}/usr/lib64 %{buildroot}/usr/bin/uke-control setup-cdc apply --json 2>&1)
status=$?
set -e
test "$status" = 1
printf '%s' "$output" | jq -e 'tostring | contains("Setup CDC requires root")' >/dev/null
for unit in uke-control-setup-cdc.service uke-control-setup-console.service uke-control-setup-cdc-finish.service uke-control-setup-cdc-finish.path;do
 cmp "%{_builddir}/uke-control-setup-cdc/packaging/$unit" "%{buildroot}/usr/lib/systemd/system/$unit"
done
test ! -e %{buildroot}/etc/uke-control/first-setup-cdc
)
# UKE_SETUP_CDC23_CHECK_END
SPEC
cat > "$work/files" <<'SPEC'
# UKE_SETUP_CDC23_FILES_BEGIN
/usr/lib/systemd/system/uke-control-setup-cdc.service
/usr/lib/systemd/system/uke-control-setup-console.service
/usr/lib/systemd/system/uke-control-setup-cdc-finish.service
/usr/lib/systemd/system/uke-control-setup-cdc-finish.path
/usr/lib/udev/rules.d/91-uke-control-setup-cdc.rules
/usr/lib/systemd/system-preset/81-uke-control-setup-cdc.preset
# UKE_SETUP_CDC23_FILES_END
SPEC
awk -v base="$work" '
 function inject(name,path,line){path=base"/"name;while((getline line<path)>0)print line;close(path)}
 /^%changelog$/{history=1}
 !history{
  if($0=="Release: 22%{?dist}"){$0="Release: 23%{?dist}";releases++}
  versions+=gsub(/2[.]17[.]0-22/,"2.17.0-23")
  if($0=="%description"&&!header++){inject("header")}
  if($0=="%build"){inject("prep");prep++;print;inject("build");build++;next}
  if($0=="install -Dm755 build/uke-control %{buildroot}/usr/bin/uke-control"){print;inject("install");installs++;next}
  if($0=="%check"){print;inject("check");checks++;next}
  if($0=="%post -n uke-control"){print;print "# UKE_SETUP_CDC23_POST_BEGIN";print "%systemd_post uke-control-setup-cdc.service uke-control-setup-console.service uke-control-setup-cdc-finish.service uke-control-setup-cdc-finish.path";print "# UKE_SETUP_CDC23_POST_END";posts++;next}
  if($0=="%preun -n uke-control"){print;print "# UKE_SETUP_CDC23_PREUN_BEGIN";print "%systemd_preun uke-control-setup-cdc.service uke-control-setup-console.service uke-control-setup-cdc-finish.service uke-control-setup-cdc-finish.path";print "# UKE_SETUP_CDC23_PREUN_END";preuns++;next}
  if($0=="%postun -n uke-control"){print;print "# UKE_SETUP_CDC23_POSTUN_BEGIN";print "%systemd_postun uke-control-setup-cdc.service uke-control-setup-console.service uke-control-setup-cdc-finish.service uke-control-setup-cdc-finish.path";print "# UKE_SETUP_CDC23_POSTUN_END";postuns++;next}
  if($0=="%files"){print;inject("files");files++;next}
 }
 {print}
 END{if(releases!=3||versions!=12||header<1||prep!=1||build!=1||installs!=1||checks!=1||posts!=1||preuns!=1||postuns!=1||files!=1)exit 1}
' "$parent" > "$result"
awk '
 /^# UKE_SETUP_CDC23_[A-Z]+_BEGIN$/{skip=1;next}
 /^# UKE_SETUP_CDC23_[A-Z]+_END$/{skip=0;next}
 skip{next}
 /^%changelog$/{history=1}
 !history{if($0=="Release: 23%{?dist}")$0="Release: 22%{?dist}";gsub(/2[.]17[.]0-23/,"2.17.0-22")}
 {print}
' "$result" | cmp - "$parent"
printf 'Control23 whole-recipe inverse retains all Control22 sources, checks, hooks, SELinux, Motion and existing ELF ownership.\n'
