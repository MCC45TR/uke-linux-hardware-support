#!/usr/bin/env bash
# COPR only: qualify trial health and bound rollback retries in the retained cohort.
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
 jq -e '(.items|length)<100 and all(.items[]; ((.source_package.version // "")|sub("[.]fc[0-9]+([.][0-9]+)*$";""))!="1.8.37-1")' "$stage/builds.json" >/dev/null
done
curl -fLsS --retry 2 --proto '=https' --proto-redir '=https' \
 https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/srpm-builds/11250994/uke-boot-integration-1.8.36-1.fc46.src.rpm \
 -o "$stage/base.src.rpm"
parent=8694a022acca7fe1009642724266237fe872d91fa55ef78c1b6c356a19a65e9d
printf '%s  %s\n' "$parent" "$stage/base.src.rpm" | sha256sum -c -
rpm2cpio "$stage/base.src.rpm" > "$stage/base.cpio"
cpio -t --quiet < "$stage/base.cpio" | awk '/^\/|(^|\/)\.\.($|\/)/ {bad=1} END {exit bad}'
mkdir -p "$stage/sources" "$stage/outer" "$stage/inner" "$output"
(cd "$stage/sources"; cpio -idm --quiet --no-absolute-filenames < "$stage/base.cpio")
unpack() {
 tar -tJf "$1" | awk '/^\/|(^|\/)\.\.($|\/)/ {bad=1} END {exit bad}'
 tar -xJf "$1" -C "$2"
}
unpack "$stage/sources/uke-boot-integration-1.8.36.tar.xz" "$stage/outer"
outer=$stage/outer/uke-boot-integration-1.8.36
(cd "$outer"; sha256sum --check --strict --quiet SOURCE-SHA256SUMS)
input=$outer/inputs/uke-boot-integration
unpack "$input/uke-boot-integration-1.8.36.tar.xz" "$stage/inner"
inner=$stage/inner/uke-boot-integration-1.8.36
(cd "$inner"; find . -type f -print0 | sort -z | xargs -0 sha256sum) > "$stage/before.sha256"
cat > "$stage/overlays" <<'FILES'
src/kernel-update.cpp
src/trial-health.hpp
src/userdata-update.hpp
src/slot-control.hpp
src/units/uke-userdata-accept.service
tests/trial-health.cpp
tests/userdata-update.cpp
tests/slot-control.cpp
tests/userdata-retry-systemd.sh
tests/update-packaging.sh
docs/KERNEL-UPDATES.md
docs/USERDATA-AB.md
packaging/uke-boot-integration.spec
FILES
while read -r path; do
 [[ $path == packaging/uke-boot-integration.spec ]] || install -Dm644 "$owner/$path" "$inner/$path"
done < "$stage/overlays"
rewrite_spec() {
 awk '
 /^%changelog/ {history=1}
 !history {gsub(/1[.]8[.]36/,"1.8.37")}
 {print}
 /^Provides: uke-kernel-update\(userdata-fallback-v1\) = 1$/ {print "Provides: uke-kernel-update(trial-health-v2) = 1"}
 /^\.\/kernel-update-contract$/ {
  print "g++ -std=c++20 -O2 -Wall -Wextra -Werror tests/trial-health.cpp -o trial-health-contract"
  print "./trial-health-contract"
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
[[ $(grep -Fc 'migrate-automatic-policy || exit 1' "$output/uke-boot-integration.spec") == 1 ]]
[[ $(grep -Fc 'bash tests/automatic-policy-rpm.sh' "$output/uke-boot-integration.spec") == 1 ]]
[[ $(grep -Fxc './trial-health-contract' "$output/uke-boot-integration.spec") == 1 ]]
commit=$(git -C "$owner" rev-parse HEAD)
epoch=$(git -C "$owner" show -s --format=%ct HEAD)
mv "$inner" "$stage/inner/uke-boot-integration-1.8.37"
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf "$input/uke-boot-integration-1.8.37.tar.xz" -C "$stage/inner" uke-boot-integration-1.8.37
unlink "$input/uke-boot-integration-1.8.36.tar.xz"
jq -n --arg commit "$commit" --arg parent "$parent" \
 '{schema:1,source_commit:$commit,parent_build:11250994,parent_srpm_sha256:$parent,
  trial_health_stable_seconds:30,trial_health_deadline_seconds:90,trial_boot_attempts:3,
  fatal_trial_health_exit_status:78,legacy_policy_migration_retained:true,
  fallback_selection_revalidated:true,firmware_payload_included:false,physical_acceptance:false}' > "$outer/rollback-health-overlay.json"
(cd "$outer"; find . -type f ! -name SOURCE-SHA256SUMS -print0 | sort -z | xargs -0 sha256sum > SOURCE-SHA256SUMS)
mv "$outer" "$stage/outer/uke-boot-integration-1.8.37"
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf "$output/uke-boot-integration-1.8.37.tar.xz" -C "$stage/outer" uke-boot-integration-1.8.37
[[ $(rpmspec --target aarch64 --define 'dist %{nil}' -q --srpm --qf '%{VERSION}-%{RELEASE}' "$output/uke-boot-integration.spec") == 1.8.37-1.fc46 ]]
