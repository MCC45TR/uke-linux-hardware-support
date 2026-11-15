#!/usr/bin/bash
# Preserve the signed20 recipe and every earlier integration through an inverse.
set -Eeuo pipefail
[[ $# == 3 ]]
before=$(realpath "$1");after=$(realpath -m "$2");contract=$(realpath "$3")
[[ ! -e $after && ! -L $after && $(jq -er .release "$contract") == 21 ]]
printf '%s  %s\n' "$(jq -er .parent.spec_sha256 "$contract")" "$before" | sha256sum -c --strict --quiet -
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
cat > "$work/sources" <<'SPEC'
# UKE_USB_API_OUTPUT_SOURCE_BEGIN
Source68: usb-api-output.patch
Source69: usb-api-output-source.json
Source70: rpm-api-output.sh
Source71: control20-parent-runtime.rpm
# UKE_USB_API_OUTPUT_SOURCE_END
SPEC
cat > "$work/prep" <<'SPEC'
# UKE_USB_API_OUTPUT_PREP_BEGIN
(
usb_root="%{_builddir}/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/work/uke-control/uke-control-2.8.1"
usb_before=$(mktemp -d)
trap 'find "$usb_before" -depth -delete' EXIT
cd "$usb_root"
cp --parents src/backend.cpp src/usb-request.cpp "$usb_before/"
printf '%s  %s\n' "$(jq -er .patch_sha256 %{SOURCE69})" %{SOURCE68} | sha256sum -c --strict --quiet -
patch -p1 --fuzz=0 --forward --dry-run -i %{SOURCE68}
patch -p1 --fuzz=0 --forward -i %{SOURCE68}
mkdir "$usb_before/inverse"
cp --parents src/backend.cpp src/usb-request.cpp "$usb_before/inverse/"
(cd "$usb_before/inverse";patch -p1 --fuzz=0 --reverse -i %{SOURCE68})
cmp "$usb_before/src/backend.cpp" "$usb_before/inverse/src/backend.cpp"
cmp "$usb_before/src/usb-request.cpp" "$usb_before/inverse/src/usb-request.cpp"
)
# UKE_USB_API_OUTPUT_PREP_END
SPEC
cat > "$work/check" <<'SPEC'
# UKE_USB_API_OUTPUT_CHECK_BEGIN
printf '%s  %s\n' "$(jq -er .fixture_sha256 %{SOURCE69})" %{SOURCE70} \
 "$(jq -er .parent.runtime_sha256 %{SOURCE69})" %{SOURCE71} | sha256sum -c --strict --quiet -
bash %{SOURCE70} "%{_builddir}/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/work/uke-control/uke-control-2.8.1" %{SOURCE71}
# UKE_USB_API_OUTPUT_CHECK_END
SPEC
awk -v sources="$work/sources" -v prep="$work/prep" -v checks="$work/check" '
 function inject(path){while((getline line<path)>0)print line;close(path)}
 /^%changelog$/{history=1}
 !history{
  if($0=="Release: 20%{?dist}"){$0="Release: 21%{?dist}";releases++}
  versions+=gsub(/2[.]17[.]0-20/,"2.17.0-21")
  if($0=="%description"&&!source_added){inject(sources);source_added++}
  if($0=="%build"){inject(prep);prep_added++}
  if($0=="%check"){print;inject(checks);check_added++;next}
 }
 {print}
 END{if(releases!=3||versions!=12||source_added!=1||prep_added!=1||check_added!=1)exit 1}
' "$before" > "$after"
awk '
 /^# UKE_USB_API_OUTPUT_(SOURCE|PREP|CHECK)_BEGIN$/{skip=1;next}
 /^# UKE_USB_API_OUTPUT_(SOURCE|PREP|CHECK)_END$/{skip=0;next}
 skip{next}
 /^%changelog$/{history=1}
 !history{if($0=="Release: 21%{?dist}")$0="Release: 20%{?dist}";gsub(/2[.]17[.]0-21/,"2.17.0-20")}
 {print}
' "$after" | cmp - "$before"
printf 'Control21 complete recipe inverse preserves20 RGB,19 Motion and all earlier sources, phases, checks, scriptlets and ownership.\n'
