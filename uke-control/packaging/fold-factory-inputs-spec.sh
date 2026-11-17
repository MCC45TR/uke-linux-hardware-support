#!/usr/bin/bash
# Preserve the complete signed Control26 recipe and existing runtime names.
set -Eeuo pipefail
[[ $# == 3 ]]
parent=$(realpath "$1");result=$(realpath -m "$2");contract=$(realpath "$3")
[[ ! -e $result && $(jq -er .release "$contract") == 27 ]]
printf '%s  %s\n' "$(jq -er .parent.spec_sha256 "$contract")" "$parent" | sha256sum -c --strict --quiet -
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
cat > "$work/header" <<'SPEC'
# UKE_FACTORY_INPUTS27_HEADER_BEGIN
Source83: factory-inputs-overlay.tar.xz
Source84: factory-inputs-source.json
BuildRequires: uke-firmware(oem-display-inputs-v1) = 1
Requires: uke-firmware(oem-display-inputs-v1) = 1
Provides: uke-control(factory-update-inputs-v1) = 1
# UKE_FACTORY_INPUTS27_HEADER_END
SPEC
cat > "$work/prep" <<'SPEC'
# UKE_FACTORY_INPUTS27_PREP_BEGIN
(
overlay=%{_builddir}/uke-control-factory-inputs
mkdir "$overlay"
tar -xJf %{SOURCE83} -C "$overlay"
(cd "$overlay";sha256sum -c --strict --quiet SHA256SUMS)
boot=%{_builddir}/uke-boot-integration-1.8.102/work/uke-boot-integration/uke-boot-integration-1.8.102
color=%{_builddir}/uke-control-2.8.1/o82-build
bash "$overlay/apply.sh" "$boot" "$color" "$overlay"
)
# UKE_FACTORY_INPUTS27_PREP_END
SPEC
cat > "$work/build" <<'SPEC'
# UKE_FACTORY_INPUTS27_BUILD_BEGIN
(
overlay=%{_builddir}/uke-control-factory-inputs
boot=%{_builddir}/uke-boot-integration-1.8.102/work/uke-boot-integration/uke-boot-integration-1.8.102
color=%{_builddir}/uke-control-2.8.1/o82-build
if ! bash "$overlay/tests/factory-input.sh" "$boot" "$color" "$overlay" > "$overlay/results.txt" 2>&1;then tail -n 100 "$overlay/results.txt";exit 1;fi
cat "$overlay/results.txt"
test "$(grep -c '^PASS: actual factory input:' "$overlay/results.txt")" = 8
)
# UKE_FACTORY_INPUTS27_BUILD_END
SPEC
cat > "$work/install" <<'SPEC'
# UKE_FACTORY_INPUTS27_INSTALL_BEGIN
overlay=%{_builddir}/uke-control-factory-inputs
install -Dm644 "$overlay/results.txt" %{buildroot}%{_docdir}/uke-control/factory-inputs27/results.txt
install -Dm644 "$overlay/docs/FACTORY-UPDATE-INPUTS.md" %{buildroot}%{_docdir}/uke-control/factory-inputs27/FACTORY-UPDATE-INPUTS.md
# UKE_FACTORY_INPUTS27_INSTALL_END
SPEC
cat > "$work/check" <<'SPEC'
# UKE_FACTORY_INPUTS27_CHECK_BEGIN
test "$(grep -c '^PASS: actual factory input:' %{buildroot}%{_docdir}/uke-control/factory-inputs27/results.txt)" = 8
cmp %{_builddir}/uke-control-factory-inputs/results.txt %{buildroot}%{_docdir}/uke-control/factory-inputs27/results.txt
# UKE_FACTORY_INPUTS27_CHECK_END
SPEC
awk -v base="$work" '
 function inject(name,path,line){path=base"/"name;while((getline line<path)>0)print line;close(path)}
 /^%changelog$/{history=1;print;print "# UKE_FACTORY_INPUTS27_HISTORY_BEGIN";print "* Tue Nov 17 2026 Uke contributors - 2.17.0-27";print "- Prepare existing private panel and video inputs from exact admitted packaged OEM sources.";print "";print "# UKE_FACTORY_INPUTS27_HISTORY_END";next}
 !history{
  if($0=="Release: 26%{?dist}"){$0="Release: 27%{?dist}";releases++}
  versions+=gsub(/2[.]17[.]0-26/,"2.17.0-27")
  if($0=="%description"&&!header++){inject("header")}
  if($0=="%build"){inject("prep");prep++;print;inject("build");build++;next}
  if($0=="install -Dm755 build/uke-control %{buildroot}/usr/bin/uke-control"){print;inject("install");installs++;next}
  if($0=="%check"){print;inject("check");checks++;next}
  if($0=="%files"){print;print "# UKE_FACTORY_INPUTS27_FILES_BEGIN";print "%{_docdir}/uke-control/factory-inputs27/";print "# UKE_FACTORY_INPUTS27_FILES_END";files++;next}
 }
 {print}
 END{if(releases!=3||versions!=12||header<1||prep!=1||build!=1||installs!=1||checks!=1||files!=1)exit 1}
' "$parent" > "$result"
awk '
 /^# UKE_FACTORY_INPUTS27_[A-Z]+_BEGIN$/{skip=1;next}
 /^# UKE_FACTORY_INPUTS27_[A-Z]+_END$/{skip=0;next}
 skip{next}
 /^%changelog$/{history=1}
 !history{if($0=="Release: 27%{?dist}")$0="Release: 26%{?dist}";gsub(/2[.]17[.]0-27/,"2.17.0-26")}
 {print}
' "$result" | cmp - "$parent"
echo 'Control27 whole-recipe inverse retains all83 parent inputs and current/legacy checks.'
