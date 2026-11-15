#!/usr/bin/bash
# Preserve the complete published firmware15 recipe and every original gate.
set -Eeuo pipefail
[[ $# == 2 ]]
parent=$(realpath "$1");result=$(realpath -m "$2")
[[ ! -e $result && ! -L $result ]]
printf '%s  %s\n' c2115d50fde7e3c2bfcc992451db7f0328e1a0188b7c7bcbe7387c760f668c0d "$parent" | sha256sum -c --strict --quiet -
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
cat > "$work/header" <<'SPEC'
# UKE_AUDIO_OOB16_HEADER_BEGIN
Source2: Forte_acdb_cal.acdb
Source3: uke-speaker-data.conf
Source4: audio-oob-20261116.json
Source5: audio-oob-cache.sh
BuildRequires: systemd
Requires(post): /usr/bin/systemd-tmpfiles
Provides: uke-firmware(generic-speaker-data-v1) = 1
# UKE_AUDIO_OOB16_HEADER_END
SPEC
cat > "$work/check" <<'SPEC'
# UKE_AUDIO_OOB16_CHECK_BEGIN
test "$(stat -c %s %{SOURCE2})" = 2838166
printf 'bd3f9b8834b80c23f86241756e263e36ddf92b2b8993ed83ff71c310af952f3e  %{SOURCE2}\n' | sha256sum -c --strict -
bash %{SOURCE5} %{SOURCE3}
install -Dm444 %{SOURCE2} %{buildroot}/usr/lib/firmware/qcom/uke/audio/Forte_acdb_cal.acdb
install -Dm644 %{SOURCE3} %{buildroot}/usr/lib/tmpfiles.d/uke-speaker-data.conf
install -Dm644 %{SOURCE4} %{buildroot}%{_docdir}/uke-firmware/audio-oob-20261116.json
cmp %{SOURCE2} %{buildroot}/usr/lib/firmware/qcom/uke/audio/Forte_acdb_cal.acdb
test "$(find %{buildroot}/usr/lib/firmware %{buildroot}/usr/share/uke-sensors -type f | wc -l)" = 219
# UKE_AUDIO_OOB16_CHECK_END
SPEC
cat > "$work/files" <<'SPEC'
# UKE_AUDIO_OOB16_FILES_BEGIN
/usr/lib/firmware/qcom/uke/audio/Forte_acdb_cal.acdb
/usr/lib/tmpfiles.d/uke-speaker-data.conf
%doc %{_docdir}/uke-firmware/audio-oob-20261116.json
# UKE_AUDIO_OOB16_FILES_END
SPEC
cat > "$work/post" <<'SPEC'
# UKE_AUDIO_OOB16_POST_BEGIN
%post
test -f /usr/lib/firmware/qcom/uke/audio/Forte_acdb_cal.acdb && test ! -L /usr/lib/firmware/qcom/uke/audio/Forte_acdb_cal.acdb || exit 1
printf 'bd3f9b8834b80c23f86241756e263e36ddf92b2b8993ed83ff71c310af952f3e  /usr/lib/firmware/qcom/uke/audio/Forte_acdb_cal.acdb\n' | sha256sum -c --strict --quiet - || exit 1
/usr/bin/systemd-tmpfiles --create /usr/lib/tmpfiles.d/uke-speaker-data.conf

# UKE_AUDIO_OOB16_POST_END
SPEC
awk -v header="$work/header" -v check="$work/check" -v files="$work/files" -v post="$work/post" '
 function inject(path){while((getline line<path)>0)print line;close(path)}
 /^Release: 15%\{\?dist\}$/{$0="Release: 16%{?dist}";release++}
 /^%description -n uke-firmware$/{inject(header);header_count++}
 /^%files -n uke-firmware$/{inject(check);print;inject(files);files_count++;next}
 /^%changelog$/{inject(post);print;print "* Mon Nov 16 2026 Uke contributors - 3.0.304.0-16\n- Deliver pinned generic speaker data and restore its compatibility cache without network access.\n";history++;next}
 {print}END{if(release!=1||header_count!=1||files_count!=1||history!=1)exit 1}
' "$parent" > "$result"
awk '
 /^# UKE_AUDIO_OOB16_[A-Z]+_BEGIN$/{skip=1;next}
 /^# UKE_AUDIO_OOB16_[A-Z]+_END$/{skip=0;next}
 skip{next}
 /^Release: 16%\{\?dist\}$/{$0="Release: 15%{?dist}"}
 /^%changelog$/{print;drop=3;next}
 drop{drop--;next}
 {print}
' "$result" | cmp - "$parent"
printf 'Complete firmware15 recipe inverse passed; original217/218-file gates precede the new data.\n'
