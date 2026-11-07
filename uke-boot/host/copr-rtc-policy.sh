#!/usr/bin/env bash
# COPR only: pair the secure RTC backend with trusted-time provisioning and checks.
set -Eeuo pipefail
export LC_ALL=C
test ! -e /etc/senemos/kernel-update.conf
[[ $(rpm --eval '%{_host_cpu}') == aarch64 && $(rpm --eval '%{fedora}') == 46 ]]
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
output=$(realpath -m "${1:?COPR result directory}")
make -C "$owner" validate
stage=$(mktemp -d)
trap 'find "$stage" -depth -delete' EXIT
for project in uke-linux-unstable uke-linux-test uke-linux; do
 curl -fLsS --get --data-urlencode ownername=mcc45tr --data-urlencode "projectname=$project" \
  --data-urlencode packagename=uke-boot-integration --data-urlencode limit=100 \
  https://copr.fedorainfracloud.org/api_3/build/list > "$stage/builds.json"
 jq -e '(.items|length)<100 and all(.items[]; ((.source_package.version // "")|sub("[.]fc[0-9]+([.][0-9]+)*$";""))!="1.8.39-1")' "$stage/builds.json" >/dev/null
done
curl -fLsS --retry 2 --proto '=https' --proto-redir '=https' \
 https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/srpm-builds/11251496/uke-boot-integration-1.8.38-1.fc46.src.rpm \
 -o "$stage/base.src.rpm"
parent=087528ce0594c9f729eca935c2104902b722ab268d525aedcbcce0b0f845b018
printf '%s  %s\n' "$parent" "$stage/base.src.rpm" | sha256sum -c -
rpm2cpio "$stage/base.src.rpm" > "$stage/base.cpio"
cpio -t --quiet < "$stage/base.cpio" | awk '/^\/|(^|\/)\.\.($|\/)/ {bad=1} END {exit bad}'
mkdir -p "$stage/sources" "$stage/outer" "$stage/inner" "$output"
(cd "$stage/sources"; cpio -idm --quiet --no-absolute-filenames < "$stage/base.cpio")
unpack() {
 tar -tJf "$1" | awk '/^\/|(^|\/)\.\.($|\/)/ {bad=1} END {exit bad}'
 tar -xJf "$1" -C "$2"
}
unpack "$stage/sources/uke-boot-integration-1.8.38.tar.xz" "$stage/outer"
outer=$stage/outer/uke-boot-integration-1.8.38
(cd "$outer"; sha256sum --check --strict --quiet SOURCE-SHA256SUMS)
input=$outer/inputs/uke-boot-integration
unpack "$input/uke-boot-integration-1.8.38.tar.xz" "$stage/inner"
inner=$stage/inner/uke-boot-integration-1.8.38
(cd "$inner"; find . -type f -print0 | sort -z | xargs -0 sha256sum) > "$stage/before.sha256"
cat > "$stage/overlays" <<'FILES'
src/install-profile.cpp
src/rtc-clock.hpp
src/userdata-update.hpp
src/units/75-uke-clock-floor.conf
src/units/uke-rtc-sync.service
src/units/uke-rtc-sync.timer
tests/rtc-clock.cpp
tests/rtc-packaging.sh
docs/KERNEL-UPDATES.md
docs/USERDATA-AB.md
docs/RTC-PERSISTENCE.md
packaging/uke-boot-integration.spec
FILES
while read -r path; do
 [[ $path == packaging/uke-boot-integration.spec ]] || install -Dm644 "$owner/$path" "$inner/$path"
done < "$stage/overlays"
rewrite_spec() {
 awk '
 /^%changelog/ {history=1}
 !history {gsub(/1[.]8[.]38/,"1.8.39")}
 /^%systemd_(post|preun|postun) uke-root-grow.service$/ {$0=$0 " uke-rtc-sync.service uke-rtc-sync.timer"}
 /systemd-update-helper (install|remove)-system-units uke-root-grow[.]service/ {sub(/uke-root-grow[.]service/,"uke-root-grow.service uke-rtc-sync.service uke-rtc-sync.timer")}
 {print}
 /^Summary: Native classification of the Uke Linux root partition$/ {print "Provides: uke-installation-profile(rtc-secure-offset-v1) = 1"}
 /^\.\/kernel-update-contract$/ {
  print "g++ -std=c++20 -O2 -Wall -Wextra -Werror tests/rtc-clock.cpp -o rtc-clock-contract"
  print "./rtc-clock-contract"
  print "bash tests/rtc-packaging.sh"
 }
 /^install -Dm644 src\/units\/uke-root-grow[.]service / {
  copy=$0;gsub(/uke-root-grow[.]service/,"uke-rtc-sync.service",copy);print copy
  copy=$0;gsub(/uke-root-grow[.]service/,"uke-rtc-sync.timer",copy);print copy
 }
 /^[^ ]*\/uke-root-grow[.]service$/ {
  copy=$0;sub(/uke-root-grow[.]service/,"uke-rtc-sync.service",copy);print copy
  copy=$0;sub(/uke-root-grow[.]service/,"uke-rtc-sync.timer",copy);print copy
 }
 /^%doc .*docs\/USERDATA-AB[.]md$/ {copy=$0;sub(/USERDATA-AB[.]md/,"RTC-PERSISTENCE.md",copy);print copy}
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
[[ $(grep -Fc 'migrate-automatic-policy || exit 1' "$output/uke-boot-integration.spec") == 1 ]]
[[ $(grep -Fc 'bash tests/automatic-policy-rpm.sh' "$output/uke-boot-integration.spec") == 1 ]]
[[ $(grep -Fxc './trial-health-contract' "$output/uke-boot-integration.spec") == 1 ]]
[[ $(grep -Fxc './rtc-clock-contract' "$output/uke-boot-integration.spec") == 1 ]]
commit=$(git -C "$owner" rev-parse HEAD)
epoch=$(git -C "$owner" show -s --format=%ct HEAD)
mv "$inner" "$stage/inner/uke-boot-integration-1.8.39"
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf "$input/uke-boot-integration-1.8.39.tar.xz" -C "$stage/inner" uke-boot-integration-1.8.39
unlink "$input/uke-boot-integration-1.8.38.tar.xz"
jq -n --arg commit "$commit" --arg parent "$parent" \
 '{schema:1,source_commit:$commit,parent_build:11251496,parent_srpm_sha256:$parent,
  rtc_trusted_utc_only:true,rtc_write_readback:true,rtc_boot_health_required:true,
  direct_pmic_or_sdam_writes:false,legacy_policy_migration_retained:true,
  rollback_health_retained:true,firmware_payload_included:false,physical_acceptance:false}' > "$outer/rtc-policy-overlay.json"
(cd "$outer"; find . -type f ! -name SOURCE-SHA256SUMS -print0 | sort -z | xargs -0 sha256sum > SOURCE-SHA256SUMS)
mv "$outer" "$stage/outer/uke-boot-integration-1.8.39"
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf "$output/uke-boot-integration-1.8.39.tar.xz" -C "$stage/outer" uke-boot-integration-1.8.39
[[ $(rpmspec --target aarch64 --define 'dist %{nil}' -q --srpm --qf '%{VERSION}-%{RELEASE}' "$output/uke-boot-integration.spec") == 1.8.39-1.fc46 ]]
