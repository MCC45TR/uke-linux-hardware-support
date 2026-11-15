#!/usr/bin/bash
# Text-only recipe transform with a complete inverse to the signed19 parent.
set -Eeuo pipefail
[[ $# == 3 ]]
before=$(realpath "$1"); after=$(realpath -m "$2"); contract=$(realpath "$3")
[[ ! -e $after && ! -L $after ]]
printf '%s  %s\n' "$(jq -er .parent.spec_sha256 "$contract")" "$before" | sha256sum -c --strict --quiet -
[[ $(jq -er .release "$contract") == 20 ]]
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
cat > "$work/sources" <<'SPEC'
# UKE_RGB_CHARGE_SOURCE_BEGIN
Source66: rgb-charge-status.patch
Source67: rgb-charge-source.json
Provides: uke-control(rgb-charge-status-v1) = 1
# UKE_RGB_CHARGE_SOURCE_END
SPEC
cat > "$work/prep" <<'SPEC'
# UKE_RGB_CHARGE_PREP_BEGIN
(
rgb_root="%{_builddir}/uke-control-2.8.1/work/uke-control/uke-control-2.8.1/work/uke-control/uke-control-2.8.1"
cd "$rgb_root"
jq -r '.files[]|.before_sha256+"  "+.path' %{SOURCE67} | sha256sum -c --strict --quiet -
printf '%s  %s\n' "$(jq -er .patch_sha256 %{SOURCE67})" %{SOURCE66} | sha256sum -c --strict --quiet -
patch -p1 --fuzz=0 --forward --dry-run -i %{SOURCE66}
patch -p1 --fuzz=0 --forward -i %{SOURCE66}
jq -r '.files[]|.after_sha256+"  "+.path' %{SOURCE67} | sha256sum -c --strict --quiet -
)
# UKE_RGB_CHARGE_PREP_END
SPEC
awk -v sources="$work/sources" -v prep="$work/prep" '
 function inject(path) {while((getline line<path)>0)print line;close(path)}
 /^%changelog$/ {history=1}
 !history {
  if($0=="Release: 19%{?dist}"){$0="Release: 20%{?dist}";releases++}
  versions+=gsub(/2[.]17[.]0-19/,"2.17.0-20")
  if($0=="%description"&&!source_added){inject(sources);source_added++}
  if($0=="%build"){inject(prep);prep_added++}
 }
 {print}
 END{if(releases!=3||versions!=12||source_added!=1||prep_added!=1)exit 1}
' "$before" > "$after"
awk '
 /^# UKE_RGB_CHARGE_(SOURCE|PREP)_BEGIN$/ {skip=1;next}
 /^# UKE_RGB_CHARGE_(SOURCE|PREP)_END$/ {skip=0;next}
 skip{next}
 /^%changelog$/{history=1}
 !history{if($0=="Release: 20%{?dist}")$0="Release: 19%{?dist}";gsub(/2[.]17[.]0-20/,"2.17.0-19")}
 {print}
' "$after" | cmp - "$before"
printf 'Control20 recipe inverse preserves all19 source phases, checks, scriptlets, file ownership and Motion integration.\n'
