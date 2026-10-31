#!/bin/bash
# COPR-only: retain the qualified boot family and enable its guarded policy.
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
 https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-test:custom:microphone-20261030/srpm-builds/11234271/uke-boot-integration-1.8.12-1.fc46.src.rpm -o "$stage/base.src.rpm"
printf '%s  %s\n' d911c85796ef0f0d6f18ee4578381d2a1489479b0fd55bf9709f0f522e12a9e7 "$stage/base.src.rpm" | sha256sum -c -
rpm2cpio "$stage/base.src.rpm" > "$stage/base.cpio"
cpio -t --quiet < "$stage/base.cpio" | awk '/^\/|(^|\/)\.\.($|\/)/ {bad=1} END {exit bad}'
mkdir -p "$stage/sources" "$stage/unpacked" "$stage/inner" "$output"
(cd "$stage/sources"; cpio -idm --quiet --no-absolute-filenames < "$stage/base.cpio")
tar -xJf "$stage/sources/uke-boot-integration-1.8.12.tar.xz" -C "$stage/unpacked"
outer=$stage/unpacked/uke-boot-integration-1.8.12
(cd "$outer"; sha256sum --check --strict --quiet SOURCE-SHA256SUMS)
input=$outer/inputs/uke-boot-integration
tar -xJf "$input/uke-boot-integration-1.8.12.tar.xz" -C "$stage/inner"
inner=$stage/inner/uke-boot-integration-1.8.12
# No native updater or slot-management implementation changes in this release.
for relative in src/kernel-update.cpp src/userdata-update.hpp src/installation-policy.hpp;do
 cmp "$inner/$relative" "$owner/$relative"
done
for relative in src/profiles/kernel-update.conf tests/update-packaging.sh docs/KERNEL-UPDATES.md docs/USERDATA-AB.md;do
 install -m644 "$owner/$relative" "$inner/$relative"
done
rewrite_spec() {
 awk '/^%changelog/ {print;print "* Sat Oct 31 2026 MCC45TR <75160848+MCC45TR@users.noreply.github.com> - 1.8.13-1";print "- Enable guarded DNF boot activation by default; retain verified inactive-slot rollback.";print "";history=1;next}
      !history {gsub(/1[.]8[.]12/,"1.8.13")} {print}' "$1" > "$2"
}
rewrite_spec "$inner/packaging/uke-boot-integration.spec" "$stage/native.spec"
install -m644 "$stage/native.spec" "$inner/packaging/uke-boot-integration.spec"
rewrite_spec "$input/uke-boot-integration.spec" "$stage/input.spec"
install -m644 "$stage/input.spec" "$input/uke-boot-integration.spec"
rewrite_spec "$stage/sources/uke-boot-integration.spec" "$output/uke-boot-integration.spec"
mv "$inner" "$stage/inner/uke-boot-integration-1.8.13"
tar --sort=name --mtime=@1793471400 --owner=0 --group=0 --numeric-owner -cJf "$input/uke-boot-integration-1.8.13.tar.xz" -C "$stage/inner" uke-boot-integration-1.8.13
unlink -- "$input/uke-boot-integration-1.8.12.tar.xz"
commit=$(git -C "$owner" rev-parse HEAD)
jq -n --arg commit "$commit" '{source_commit:$commit,parent_build:11234271,parent_srpm_sha256:"d911c85796ef0f0d6f18ee4578381d2a1489479b0fd55bf9709f0f522e12a9e7",native_implementation_unchanged:true,automatic_default:true,overlays:["src/profiles/kernel-update.conf","tests/update-packaging.sh","docs/KERNEL-UPDATES.md","docs/USERDATA-AB.md"],physical_acceptance:false}' > "$outer/automatic-policy-overlay.json"
(cd "$outer"; find . -type f ! -name SOURCE-SHA256SUMS -print0 | sort -z | xargs -0 sha256sum > SOURCE-SHA256SUMS)
mv "$outer" "$stage/unpacked/uke-boot-integration-1.8.13"
tar --sort=name --mtime=@1793471400 --owner=0 --group=0 --numeric-owner -cJf "$output/uke-boot-integration-1.8.13.tar.xz" -C "$stage/unpacked" uke-boot-integration-1.8.13
test "$(rpmspec --target aarch64 --define 'dist %{nil}' -q --srpm --qf '%{VERSION}-%{RELEASE}' "$output/uke-boot-integration.spec")" = 1.8.13-1.fc46
