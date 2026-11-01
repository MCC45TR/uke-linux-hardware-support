#!/bin/bash
# COPR-only: retain the verified updater and bound the greeter's device wait.
set -Eeuo pipefail
export LC_ALL=C
test ! -e /etc/senemos/kernel-update.conf
[[ $(rpm --eval '%{_host_cpu}') == aarch64 && $(rpm --eval '%{fedora}') == 46 ]]
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
output=$(realpath -m "${1:?COPR result directory}")
make -C "$owner" validate
stage=$(mktemp -d)
trap 'find "$stage" -depth -delete' EXIT
curl -fLsS --proto '=https' --proto-redir '=https' --retry 3 \
 https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/srpm-builds/11239520/uke-boot-integration-1.8.14-1.fc46.src.rpm -o "$stage/base.src.rpm"
printf '%s  %s\n' 435a77beb5883830273083c2407230a07c9d67bac3168297c7170e4ac88ff0f6 "$stage/base.src.rpm" | sha256sum -c -
rpm2cpio "$stage/base.src.rpm" > "$stage/base.cpio"
cpio -t --quiet < "$stage/base.cpio" | awk '/^\/|(^|\/)\.\.($|\/)/ {bad=1} END {exit bad}'
mkdir -p "$stage/sources" "$stage/unpacked" "$stage/inner" "$output"
(cd "$stage/sources"; cpio -idm --quiet --no-absolute-filenames < "$stage/base.cpio")
tar -xJf "$stage/sources/uke-boot-integration-1.8.14.tar.xz" -C "$stage/unpacked"
outer=$stage/unpacked/uke-boot-integration-1.8.14
(cd "$outer"; sha256sum --check --strict --quiet SOURCE-SHA256SUMS)
input=$outer/inputs/uke-boot-integration
tar -xJf "$input/uke-boot-integration-1.8.14.tar.xz" -C "$stage/inner"
inner=$stage/inner/uke-boot-integration-1.8.14
for relative in src/kernel-update.cpp src/userdata-update.hpp src/installation-policy.hpp src/units/uke-userdata-accept.service; do
 cmp "$inner/$relative" "$owner/$relative"
done
for relative in src/units/30-uke-plasmalogin-ready.conf docs/KERNEL-UPDATES.md; do
 install -m644 "$owner/$relative" "$inner/$relative"
done
rewrite_spec() {
 awk '/^%changelog/ {print;print "* Sun Nov 01 2026 MCC45TR <75160848+MCC45TR@users.noreply.github.com> - 1.8.15-1";print "- Complete the greeter wait when native DRM is available; retain updater verification.";print "- Use a relative firmware alias and the pinned commit archive timestamp.";print "";history=1;next}
      !history {
        gsub(/1[.]8[.]14/,"1.8.15")
        gsub(/ln -s \/usr\/lib\/firmware\/qcom\/uke\/touch\/csot\/normal[.]bin/,"ln -s ../../qcom/uke/touch/csot/normal.bin")
        if(/^install -Dm644 src\/units\/70-uke-chronyd-clock.conf /) {
          print;print "install -Dm644 src/units/30-uke-plasmalogin-ready.conf %{buildroot}%{_unitdir}/plasmalogin.service.d/30-uke-native-drm.conf";next
        }
        if(!/^install / && /\/chronyd[.]service[.]d\/70-uke-clock[.]conf$/) {
          print;print "%dir %{_unitdir}/plasmalogin.service.d";print "%{_unitdir}/plasmalogin.service.d/30-uke-native-drm.conf";next
        }
      } {print}' "$1" > "$2"
 test "$(grep -c '^install -Dm644 src/units/30-uke-plasmalogin-ready.conf ' "$2")" = 1
 test "$(grep -c '^%{_unitdir}/plasmalogin.service.d/30-uke-native-drm.conf$' "$2")" = 1
}
rewrite_spec "$inner/packaging/uke-boot-integration.spec" "$stage/native.spec"
install -m644 "$stage/native.spec" "$inner/packaging/uke-boot-integration.spec"
rewrite_spec "$input/uke-boot-integration.spec" "$stage/input.spec"
install -m644 "$stage/input.spec" "$input/uke-boot-integration.spec"
rewrite_spec "$stage/sources/uke-boot-integration.spec" "$output/uke-boot-integration.spec"
commit=$(git -C "$owner" rev-parse HEAD)
epoch=$(git -C "$owner" show -s --format=%ct HEAD)
mv "$inner" "$stage/inner/uke-boot-integration-1.8.15"
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf "$input/uke-boot-integration-1.8.15.tar.xz" -C "$stage/inner" uke-boot-integration-1.8.15
unlink -- "$input/uke-boot-integration-1.8.14.tar.xz"
jq -n --arg commit "$commit" '{source_commit:$commit,parent_build:11239520,parent_srpm_sha256:"435a77beb5883830273083c2407230a07c9d67bac3168297c7170e4ac88ff0f6",functional_changes:["src/units/30-uke-plasmalogin-ready.conf"],native_updater_unchanged:true,relative_firmware_alias:true,archive_epoch:"pinned source commit",physical_acceptance:false}' > "$outer/greeter-readiness-overlay.json"
(cd "$outer"; find . -type f ! -name SOURCE-SHA256SUMS -print0 | sort -z | xargs -0 sha256sum > SOURCE-SHA256SUMS)
mv "$outer" "$stage/unpacked/uke-boot-integration-1.8.15"
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf "$output/uke-boot-integration-1.8.15.tar.xz" -C "$stage/unpacked" uke-boot-integration-1.8.15
test "$(rpmspec --target aarch64 --define 'dist %{nil}' -q --srpm --qf '%{VERSION}-%{RELEASE}' "$output/uke-boot-integration.spec")" = 1.8.15-1.fc46
