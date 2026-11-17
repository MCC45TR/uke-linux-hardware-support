#!/usr/bin/bash
# Extend the complete firmware17 recipe without changing any existing input.
set -Eeuo pipefail
[[ $# == 2 ]]
parent=$(realpath "$1");result=$(realpath -m "$2")
[[ ! -e $result && ! -L $result ]]
printf '%s  %s\n' 3b168826e82dbe16bd0191f9b8c76492fead6c57d8d611046cf6504f2be1e387 "$parent" | sha256sum -c --strict --quiet -
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
cat > "$work/header" <<'SPEC'
# UKE_DISPLAY18_HEADER_BEGIN
Source8: o82-36.json
Source9: o82-42.json
Source10: vpu30_2v.mbn
Source11: display-inputs-20261117.json
Provides: uke-firmware(oem-display-inputs-v1) = 1
# UKE_DISPLAY18_HEADER_END
SPEC
cat > "$work/check" <<'SPEC'
# UKE_DISPLAY18_CHECK_BEGIN
for tuple in '8 12385898 bd88eb94134e5f62c05eebaa0a9e4b5267968363f72f440018b98a15d1bd9122' '9 12383296 2b6442ce0e8fef6d1b008cdeca3838450282868d385f6e3056c0a4ce2b8949d8' '10 2331336 3a4cac666ed1cd1ab5cb55f975dc724fd3e10603713b382f390484c37548d231';do
 read -r index size hash <<< "$tuple"
 case $index in 8) input=%{SOURCE8};;9) input=%{SOURCE9};;10) input=%{SOURCE10};;*) exit 1;;esac
 test "$(stat -c %s "$input")" = "$size"
 printf '%s  %s\n' "$hash" "$input" | sha256sum -c --strict -
done
install -Dm444 %{SOURCE8} %{buildroot}/usr/lib/firmware/qcom/uke/display/o82-36.json
install -Dm444 %{SOURCE9} %{buildroot}/usr/lib/firmware/qcom/uke/display/o82-42.json
install -Dm444 %{SOURCE10} %{buildroot}/usr/lib/firmware/qcom/uke/video/vpu30_2v.mbn
install -Dm644 %{SOURCE11} %{buildroot}%{_docdir}/uke-firmware/display-inputs-20261117.json
cmp %{SOURCE8} %{buildroot}/usr/lib/firmware/qcom/uke/display/o82-36.json
cmp %{SOURCE9} %{buildroot}/usr/lib/firmware/qcom/uke/display/o82-42.json
cmp %{SOURCE10} %{buildroot}/usr/lib/firmware/qcom/uke/video/vpu30_2v.mbn
test "$(find %{buildroot}/usr/lib/firmware %{buildroot}/usr/share/uke-sensors -type f | wc -l)" = 223
# UKE_DISPLAY18_CHECK_END
SPEC
cat > "$work/files" <<'SPEC'
# UKE_DISPLAY18_FILES_BEGIN
/usr/lib/firmware/qcom/uke/display/o82-36.json
/usr/lib/firmware/qcom/uke/display/o82-42.json
/usr/lib/firmware/qcom/uke/video/vpu30_2v.mbn
%doc %{_docdir}/uke-firmware/display-inputs-20261117.json
# UKE_DISPLAY18_FILES_END
SPEC
awk -v base="$work" '
 function inject(name,path,line){path=base"/"name;while((getline line<path)>0)print line;close(path)}
 /^Release: 17%\{\?dist\}$/{$0="Release: 18%{?dist}";release++}
 /^%description -n uke-firmware$/{inject("header");headers++}
 /^%files -n uke-firmware$/{inject("check");print;inject("files");files++;next}
 /^%changelog$/{print;print "# UKE_DISPLAY18_HISTORY_BEGIN";print "* Tue Nov 17 2026 Uke contributors - 3.0.304.0-18";print "- Retain already admitted generic OEM panel and video inputs independently of Android super.";print "";print "# UKE_DISPLAY18_HISTORY_END";next}
 {print}END{if(release!=1||headers!=1||files!=1)exit 1}
' "$parent" > "$result"
awk '
 /^# UKE_DISPLAY18_[A-Z]+_BEGIN$/{skip=1;next}
 /^# UKE_DISPLAY18_[A-Z]+_END$/{skip=0;next}
 skip{next}
 /^Release: 18%\{\?dist\}$/{$0="Release: 17%{?dist}"}
 {print}
' "$result" | cmp - "$parent"
[[ $(rpmspec --target aarch64 --define 'dist .fc46' -q --srpm --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$result") == uke-firmware-3.0.304.0-18.fc46 ]]
