#!/usr/bin/bash
# Preserve the signed21 recipe, both USB API budgets, RGB and all prior features.
set -Eeuo pipefail
[[ $# == 2 ]]
parent=$(realpath "$1");result=$(realpath -m "$2")
[[ ! -e $result && ! -L $result ]]
printf '%s  %s\n' 97eb4b26c3793077d6d4c537a74d378658dcf5f2c2902b7b445b6483f3c903c5 "$parent" | sha256sum -c --strict --quiet -
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
cat > "$work/header" <<'SPEC'
# UKE_BOOT_IDENTITY22_HEADER_BEGIN
Source72: boot-status-unstable.patch
Source73: boot-status-unstable.sh
# UKE_BOOT_IDENTITY22_HEADER_END
SPEC
cat > "$work/prep" <<'SPEC'
# UKE_BOOT_IDENTITY22_PREP_BEGIN
(
cd "%{_builddir}/uke-boot-integration-1.8.102/work/uke-boot-integration/uke-boot-integration-1.8.102"
printf 'e0dced833f7f93624df04a7519eae34a3becb97fdd022906796b609df6bcef71  src/boot-status.cpp\n60bf5d118bc5fc88629358118aafd3aa17dc98824ce3589f1214c94a0d719192  src/unstable-release.hpp\n' | sha256sum -c --strict --quiet -
boot_before=$(mktemp -d)
trap 'find "$boot_before" -depth -delete' EXIT
cp --parents src/boot-status.cpp tests/boot-status.sh "$boot_before/"
patch -p1 --fuzz=0 --forward --dry-run -i %{SOURCE72}
patch -p1 --fuzz=0 --forward -i %{SOURCE72}
printf '92089693d0373683a64238c88be8989588faf0909a02c1737963d392d206e394  src/boot-status.cpp\n' | sha256sum -c --strict --quiet -
mkdir "$boot_before/inverse"
cp --parents src/boot-status.cpp tests/boot-status.sh "$boot_before/inverse/"
(cd "$boot_before/inverse";patch -p1 --fuzz=0 --reverse -i %{SOURCE72})
cmp "$boot_before/src/boot-status.cpp" "$boot_before/inverse/src/boot-status.cpp"
cmp "$boot_before/tests/boot-status.sh" "$boot_before/inverse/tests/boot-status.sh"
)
# UKE_BOOT_IDENTITY22_PREP_END
SPEC
cat > "$work/check" <<'SPEC'
# UKE_BOOT_IDENTITY22_CHECK_BEGIN
bash %{SOURCE73} "%{_builddir}/uke-boot-integration-1.8.102/work/uke-boot-integration/uke-boot-integration-1.8.102" %{buildroot}/usr/bin/uke-boot-status
# UKE_BOOT_IDENTITY22_CHECK_END
SPEC
awk -v header="$work/header" -v prep="$work/prep" -v checks="$work/check" '
 function inject(path){while((getline line<path)>0)print line;close(path)}
 /^%changelog$/{history=1}
 !history{
  if($0=="Release: 21%{?dist}"){$0="Release: 22%{?dist}";releases++}
  versions+=gsub(/2[.]17[.]0-21/,"2.17.0-22")
  if($0=="%description"&&!header_added){inject(header);header_added++}
  if($0=="%build"){inject(prep);prep_added++}
  if($0=="%check"){print;inject(checks);check_added++;next}
  if(/src\/boot-status[.]cpp -o uke-boot-status/){if(gsub(/-std=c[+][+]17/,"-std=c++20")!=1)exit 1;compiler++}
 }
 {print}END{if(releases!=3||versions!=12||header_added!=1||prep_added!=1||check_added!=1||compiler!=1)exit 1}
' "$parent" > "$result"
awk '
 /^# UKE_BOOT_IDENTITY22_(HEADER|PREP|CHECK)_BEGIN$/{skip=1;next}
 /^# UKE_BOOT_IDENTITY22_(HEADER|PREP|CHECK)_END$/{skip=0;next}
 skip{next}
 /^%changelog$/{history=1}
 !history{
  if($0=="Release: 22%{?dist}")$0="Release: 21%{?dist}"
  gsub(/2[.]17[.]0-22/,"2.17.0-21")
  if(/src\/boot-status[.]cpp -o uke-boot-status/)gsub(/-std=c[+][+]20/,"-std=c++17")
 }{print}
' "$result" | cmp - "$parent"
printf 'Complete Control21 recipe inverse passed; only boot-status adopts the existing canonical parser and its C++20 requirement.\n'
