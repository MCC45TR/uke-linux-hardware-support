#!/usr/bin/bash
# Preserve the complete16 recipe, including all old audio and file-count gates.
set -Eeuo pipefail
[[ $# == 2 ]]
parent=$(realpath "$1");result=$(realpath -m "$2")
[[ ! -e $result && ! -L $result ]]
printf '%s  %s\n' 185c46ee7cc17654b737a242e8dbfaf60e2908e3c68080d1be2bf5759b2df84f "$parent" | sha256sum -c --strict --quiet -
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
cat > "$work/header" <<'SPEC'
# UKE_MCU17_HEADER_BEGIN
Source6: MCU_Upgrade.bin
Source7: pogo-mcu-20261116.json
Provides: uke-firmware(factory-pogo-mcu-v1) = 1
# UKE_MCU17_HEADER_END
SPEC
cat > "$work/check" <<'SPEC'
# UKE_MCU17_CHECK_BEGIN
test "$(stat -c %s %{SOURCE6})" = 82740
printf 'b17fa938cf5613dfa550179d6fc40cfbaa831d534a16a22282e2904e845fa7c4  %{SOURCE6}\n' | sha256sum -c --strict -
install -Dm444 %{SOURCE6} %{buildroot}/usr/lib/firmware/qcom/uke/nanosic/MCU_Upgrade.bin
install -Dm644 %{SOURCE7} %{buildroot}%{_docdir}/uke-firmware/pogo-mcu-20261116.json
cmp %{SOURCE6} %{buildroot}/usr/lib/firmware/qcom/uke/nanosic/MCU_Upgrade.bin
test "$(find %{buildroot}/usr/lib/firmware %{buildroot}/usr/share/uke-sensors -type f | wc -l)" = 220
# UKE_MCU17_CHECK_END
SPEC
cat > "$work/files" <<'SPEC'
# UKE_MCU17_FILES_BEGIN
/usr/lib/firmware/qcom/uke/nanosic/MCU_Upgrade.bin
%doc %{_docdir}/uke-firmware/pogo-mcu-20261116.json
# UKE_MCU17_FILES_END
SPEC
awk -v header="$work/header" -v check="$work/check" -v files="$work/files" '
 function inject(path){while((getline line<path)>0)print line;close(path)}
 /^Release: 16%\{\?dist\}$/{$0="Release: 17%{?dist}";release++}
 /^%description -n uke-firmware$/{inject(header);header_count++}
 /^%files -n uke-firmware$/{inject(check);print;inject(files);files_count++;next}
 /^%changelog$/{print;print "* Mon Nov 16 2026 Uke contributors - 3.0.304.0-17\n- Supply the exact admitted public MCU image independently of Android super.\n";history++;next}
 {print}END{if(release!=1||header_count!=1||files_count!=1||history!=1)exit 1}
' "$parent" > "$result"
awk '
 /^# UKE_MCU17_[A-Z]+_BEGIN$/{skip=1;next}
 /^# UKE_MCU17_[A-Z]+_END$/{skip=0;next}
 skip{next}
 /^Release: 17%\{\?dist\}$/{$0="Release: 16%{?dist}"}
 /^%changelog$/{print;drop=3;next}
 drop{drop--;next}
 {print}
' "$result" | cmp - "$parent"
printf 'Complete firmware16 recipe inverse passed; all217/218/219-file and offline audio gates retained.\n'
