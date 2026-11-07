#!/usr/bin/env bash
# COPR only: retain the published RTC/touch/rollback cohort and add slot selection.
set -Eeuo pipefail
export LC_ALL=C
test ! -e /etc/senemos/kernel-update.conf
[[ $(rpm --eval '%{_host_cpu}') == aarch64 && $(rpm --eval '%{fedora}') == 46 ]]
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
output=$(realpath -m "${1:?COPR result directory}")
stage=$(mktemp -d)
trap 'find "$stage" -depth -delete' EXIT
for project in uke-linux-unstable uke-linux-test uke-linux; do
 curl -fLsS --get --data-urlencode ownername=mcc45tr --data-urlencode "projectname=$project" \
  --data-urlencode packagename=uke-boot-integration --data-urlencode limit=100 \
  https://copr.fedorainfracloud.org/api_3/build/list > "$stage/builds.json"
 jq -e '(.items|length)<100 and all(.items[]; ((.source_package.version // "")|sub("[.]fc[0-9]+([.][0-9]+)*$";""))!="1.8.40-1")' "$stage/builds.json" >/dev/null
done
curl -fLsS --retry 2 --proto '=https' --proto-redir '=https' \
 https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/srpm-builds/11251582/uke-boot-integration-1.8.39-1.fc46.src.rpm \
 -o "$stage/base.src.rpm"
parent=2d553e6ea908e0a66b63090b5a5f04e59a44cccf12e0dca16a827888d29011db
printf '%s  %s\n' "$parent" "$stage/base.src.rpm" | sha256sum -c -
rpm2cpio "$stage/base.src.rpm" > "$stage/base.cpio"
cpio -t --quiet < "$stage/base.cpio" | awk '/^\/|(^|\/)\.\.($|\/)/ {bad=1} END {exit bad}'
mkdir -p "$stage/sources" "$stage/outer" "$stage/inner" "$output"
(cd "$stage/sources"; cpio -idm --quiet --no-absolute-filenames < "$stage/base.cpio")
unpack() {
 tar -tJf "$1" | awk '/^\/|(^|\/)\.\.($|\/)/ {bad=1} END {exit bad}'
 tar -xJf "$1" -C "$2"
}
unpack "$stage/sources/uke-boot-integration-1.8.39.tar.xz" "$stage/outer"
outer=$stage/outer/uke-boot-integration-1.8.39
(cd "$outer"; sha256sum --check --strict --quiet SOURCE-SHA256SUMS)
input=$outer/inputs/uke-boot-integration
unpack "$input/uke-boot-integration-1.8.39.tar.xz" "$stage/inner"
inner=$stage/inner/uke-boot-integration-1.8.39
(cd "$inner"; find . -type f -print0 | sort -z | xargs -0 sha256sum) > "$stage/before.sha256"
cat > "$stage/overlays" <<'FILES'
src/kernel-update.cpp
src/slot-selection.hpp
src/select-existing-slot.hpp
tests/slot-selection.cpp
packaging/uke-boot-integration.spec
FILES
while read -r path; do
 [[ $path == packaging/uke-boot-integration.spec ]] || install -Dm644 "$owner/$path" "$inner/$path"
done < "$stage/overlays"
rewrite_spec() {
 awk '
 /^%changelog/ {history=1;print;print "* Wed Nov 04 2026 Uke contributors - 1.8.40-1";print "- Select verified existing A/B kernels through the guarded metadata transaction.";print "- Keep boot images, RTC, touch health and automatic recovery safeguards intact.";print "";next}
 !history {gsub(/1[.]8[.]39/,"1.8.40")}
 {print}
 /^%package -n uke-kernel-update$/ {print "Provides: uke-kernel-update(slot-selection-v1) = 1"}
 /^\.\/kernel-update-contract$/ {
  print "g++ -std=c++20 -O2 -Wall -Wextra -Werror tests/slot-selection.cpp -o slot-selection-contract"
  print "./slot-selection-contract"
 }
 ' "$1" > "$2"
}
for spec in "$inner/packaging/uke-boot-integration.spec" "$input/uke-boot-integration.spec"; do
 rewrite_spec "$spec" "$stage/updated.spec";install -m644 "$stage/updated.spec" "$spec"
done
while read -r hash path; do
 path=${path#./}
 if ! grep -Fxq "$path" "$stage/overlays"; then printf '%s  %s\n' "$hash" "$inner/$path" | sha256sum -c --quiet -;fi
done < "$stage/before.sha256"
rewrite_spec "$stage/sources/uke-boot-integration.spec" "$output/uke-boot-integration.spec"
for contract in './trial-health-contract' './rtc-clock-contract' './slot-selection-contract'; do [[ $(grep -Fxc "$contract" "$output/uke-boot-integration.spec") == 1 ]]; done
[[ $(grep -Fc 'migrate-automatic-policy || exit 1' "$output/uke-boot-integration.spec") == 1 ]]
commit=$(git -C "$owner" rev-parse HEAD)
epoch=$(git -C "$owner" show -s --format=%ct HEAD)
mv "$inner" "$stage/inner/uke-boot-integration-1.8.40"
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf "$input/uke-boot-integration-1.8.40.tar.xz" -C "$stage/inner" uke-boot-integration-1.8.40
unlink "$input/uke-boot-integration-1.8.39.tar.xz"
jq -n --arg commit "$commit" --arg parent "$parent" \
 '{schema:1,source_commit:$commit,parent_build:11251582,parent_srpm_sha256:$parent,
  existing_images_only:true,automatic_reboot:false,early_firmware_writes:false,
  rtc_and_touch_health_retained:true,physical_boot_tested:false}' > "$outer/slot-selection-overlay.json"
(cd "$outer"; find . -type f ! -name SOURCE-SHA256SUMS -print0 | sort -z | xargs -0 sha256sum > SOURCE-SHA256SUMS)
mv "$outer" "$stage/outer/uke-boot-integration-1.8.40"
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf "$output/uke-boot-integration-1.8.40.tar.xz" -C "$stage/outer" uke-boot-integration-1.8.40
[[ $(rpmspec --target aarch64 --define 'dist %{nil}' -q --srpm --qf '%{VERSION}-%{RELEASE}' "$output/uke-boot-integration.spec") == 1.8.40-1.fc46 ]]
