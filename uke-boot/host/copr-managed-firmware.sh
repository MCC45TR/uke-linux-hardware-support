#!/usr/bin/env bash
# COPR only: extend the exact qualified 1.8.95 native updater source.
set -Eeuo pipefail
test ! -e /etc/senemos/kernel-update.conf
export LC_ALL=C
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
manifest=$owner/manifests/managed-firmware-20261105.json
output=$(realpath -m "${1:?Pass the COPR source result directory}")
(cd "$owner"; jq -r '.inputs[]|.sha256+"  "+.path' "$manifest" | sha256sum -c --strict --quiet -)
version=$(jq -er .version "$manifest")
[[ $version == 1.8.97 ]]
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
mkdir -p "$output" "$work/rpm" "$work/outer" "$work/inner"
for project in uke-linux uke-linux-test uke-linux-unstable; do
    curl -q -fLsS --max-time 45 --get --data-urlencode ownername=mcc45tr \
        --data-urlencode "projectname=$project" --data-urlencode packagename=uke-boot-integration \
        --data-urlencode limit=200 https://copr.fedorainfracloud.org/api_3/build/list > "$work/builds.json"
    jq -e '.items|type=="array"' "$work/builds.json" > /dev/null
    while IFS= read -r published; do
        [[ $published =~ ^[0-9]+([.][0-9]+)*-[0-9]+([.][a-zA-Z0-9]+)*$ ]]
        comparison=$(rpm --eval "%{lua:print(rpm.vercmp(\"$published\", \"$version-1.fc46\"))}")
        [[ $comparison =~ ^-?[0-9]+$ && $comparison -lt 0 ]]
    done < <(jq -r --arg project "$project" --arg candidate "$version-1.fc46" \
        '.items[]|select(.source_package.name=="uke-boot-integration" and
        .source_package.url!=null)|select(.project_dirname==$project or
        .source_package.version==$candidate)|.source_package.version' "$work/builds.json")
done
parent=$owner/$(jq -er .parent.path "$manifest")
printf '%s  %s\n' "$(jq -er .parent.sha256 "$manifest")" "$parent" | sha256sum -c --strict -
# COPR source RPMs are unsigned; the immutable source capsule is hash-pinned.
rpmkeys --checksig --nosignature --verbose "$parent"
rpm2cpio "$parent" > "$work/parent.cpio"
cpio -t --quiet < "$work/parent.cpio" > "$work/entries"
awk '/^\/|(^|\/)\.\.($|\/)/ {bad=1} END {exit bad}' "$work/entries"
(cd "$work/rpm"; cpio -idm --quiet --no-absolute-filenames < "$work/parent.cpio")
extract() {
    tar -tJf "$1" > "$work/entries"
    awk '/^\/|(^|\/)\.\.($|\/)/ {bad=1} END {exit bad}' "$work/entries"
    tar -xJf "$1" -C "$2"
}
extract "$work/rpm/uke-boot-integration-1.8.95.tar.xz" "$work/outer"
outer=$work/outer/uke-boot-integration-1.8.95
extract "$outer/inputs/uke-boot-integration/uke-boot-integration-1.8.95.tar.xz" "$work/inner"
native=$work/inner/uke-boot-integration-1.8.95
(cd "$native"; find . -type f ! -path ./src/kernel-update.cpp -print0 |
    sort -z | xargs -0 sha256sum) > "$work/retained-native.sha256"
printf '%s  %s\n' "$(jq -er .updater_before_sha256 "$manifest")" "$native/src/kernel-update.cpp" |
    sha256sum -c --strict -
git -C "$native" apply --check "$owner/patches/managed-firmware-activation.patch"
git -C "$native" apply "$owner/patches/managed-firmware-activation.patch"
printf '%s  %s\n' "$(jq -er .updater_after_sha256 "$manifest")" "$native/src/kernel-update.cpp" |
    sha256sum -c --strict -
for asset in src/managed-firmware.hpp tests/managed-firmware.cpp tests/managed-firmware-boot.cpp tests/managed-firmware.sh docs/MANAGED-FIRMWARE.md; do
    test ! -e "$native/$asset"
    install -Dm644 "$owner/$asset" "$native/$asset"
done
(cd "$native"; sha256sum -c --strict --quiet "$work/retained-native.sha256")
[[ $(cat "$native/src/profiles/rollback.conf") == mode=enforced ]]
[[ $(grep -c 'uke_color::with_prepared' "$native/src/kernel-update.cpp") == 3 ]]
[[ $(grep -c 'uke_firmware::with_prepared' "$native/src/kernel-update.cpp") == 4 ]]
[[ $(grep -c 'uke_firmware::prepared_accept' "$native/src/kernel-update.cpp") == 2 ]]
grep -Fq 'if(action!="fallback" && !restore)' "$native/src/kernel-update.cpp"
grep -Fq 'saved.policy.candidate}},uke_firmware::dry_action(action)' "$native/src/kernel-update.cpp"
printf 'All other qualified1.8.95 sources, tests, RTC, private color and persistent policies retained byte-for-byte\n'
bash "$native/tests/managed-firmware.sh"

spec=$output/uke-boot-integration.spec
awk '
 /^%changelog$/ {
    history=1;print;print "* Thu Nov 05 2026 Uke contributors - 1.8.97-1";
    print "- Require verified private firmware before managed candidate activation.";
    print "- Preserve legacy recovery, automatic staging and persistent preferences.";print "";next
 }
 !history {gsub(/1[.]8[.]95/,"1.8.97")}
 /^%package -n uke-kernel-update$/ {
    print;print "Requires: uke-cdsp-pogo-firmware >= 0.2-8";
    print "Provides: uke-kernel-update(cdsp-pogo-firmware-v1) = 1";dependency++;next
 }
 /^%check$/ {checking=1}
 checking && /^%[a-z]/ && $0!="%check" {
    print "bash %{_builddir}/uke-boot-integration-1.8.97/work/uke-boot-integration/uke-boot-integration-1.8.97/tests/managed-firmware.sh";
    checking=0;checks++
 }
 /^%files -n uke-kernel-update$/ {updater_files=1}
 updater_files && /^%doc .*\/docs\/RTC-PERSISTENCE.md$/ {
    print;print "%doc work/uke-boot-integration/uke-boot-integration-1.8.97/docs/MANAGED-FIRMWARE.md";
    updater_files=0;documentation++;next
 }
 /^%transfiletriggerin -n uke-kernel-update -- \/usr\/lib\/senemos$/ {trigger++}
 {print}
 END {if(dependency!=1||checks!=1||documentation!=1||trigger!=1)exit 1}
' "$work/rpm/uke-boot-integration.spec" > "$spec"
grep -Fxq '%config(noreplace) /etc/senemos/rollback.conf' "$spec"
grep -Fxq 'Provides: uke-kernel-update(o82-private-color-v5) = 1' "$spec"
mv "$native" "$work/inner/uke-boot-integration-$version"
epoch=$(git -C "$owner" show -s --format=%ct HEAD)
find "$outer/inputs/uke-boot-integration" -maxdepth 1 -type f -name uke-boot-integration-1.8.95.tar.xz -delete
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf \
    "$outer/inputs/uke-boot-integration/uke-boot-integration-$version.tar.xz" -C "$work/inner" "uke-boot-integration-$version"
cp "$spec" "$outer/inputs/uke-boot-integration/uke-boot-integration.spec"
install -m644 "$manifest" "$outer/managed-firmware-source.json"
git -C "$owner" rev-parse HEAD > "$outer/managed-firmware-source-commit"
mv "$outer" "$work/outer/uke-boot-integration-$version"
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf \
    "$output/uke-boot-integration-$version.tar.xz" -C "$work/outer" "uke-boot-integration-$version"
[[ $(rpmspec --define 'dist %{nil}' -q --srpm --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$spec") == uke-boot-integration-1.8.97-1.fc46 ]]
printf 'Managed firmware source prepared; complete native build/package checks and tablet admission remain required\n'
