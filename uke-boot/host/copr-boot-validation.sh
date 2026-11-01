#!/bin/bash
# COPR-only: preserve the published boot family and remove repeated validation.
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
 https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/srpm-builds/11238010/uke-boot-integration-1.8.13-1.fc46.src.rpm -o "$stage/base.src.rpm"
printf '%s  %s\n' f00ebb5a919664bd1b8baff32b8e4f6211b879e0f74d802bb19603be139e8652 "$stage/base.src.rpm" | sha256sum -c -
rpm2cpio "$stage/base.src.rpm" > "$stage/base.cpio"
cpio -t --quiet < "$stage/base.cpio" | awk '/^\/|(^|\/)\.\.($|\/)/ {bad=1} END {exit bad}'
mkdir -p "$stage/sources" "$stage/unpacked" "$stage/inner" "$output"
(cd "$stage/sources"; cpio -idm --quiet --no-absolute-filenames < "$stage/base.cpio")
tar -xJf "$stage/sources/uke-boot-integration-1.8.13.tar.xz" -C "$stage/unpacked"
outer=$stage/unpacked/uke-boot-integration-1.8.13
(cd "$outer"; sha256sum --check --strict --quiet SOURCE-SHA256SUMS)
input=$outer/inputs/uke-boot-integration
tar -xJf "$input/uke-boot-integration-1.8.13.tar.xz" -C "$stage/inner"
inner=$stage/inner/uke-boot-integration-1.8.13
for relative in src/userdata-update.hpp src/installation-policy.hpp src/units/uke-userdata-accept.service; do
 cmp "$inner/$relative" "$owner/$relative"
done
for relative in src/kernel-update.cpp docs/KERNEL-UPDATES.md; do
 install -m644 "$owner/$relative" "$inner/$relative"
done
rewrite_spec() {
 awk '/^%changelog/ {print;print "* Sun Nov 01 2026 MCC45TR <75160848+MCC45TR@users.noreply.github.com> - 1.8.14-1";print "- Reuse the verified running bundle during guarded automatic activation.";print "";history=1;next}
      !history {gsub(/1[.]8[.]13/,"1.8.14")} {print}' "$1" > "$2"
}
rewrite_spec "$inner/packaging/uke-boot-integration.spec" "$stage/native.spec"
install -m644 "$stage/native.spec" "$inner/packaging/uke-boot-integration.spec"
rewrite_spec "$input/uke-boot-integration.spec" "$stage/input.spec"
install -m644 "$stage/input.spec" "$input/uke-boot-integration.spec"
rewrite_spec "$stage/sources/uke-boot-integration.spec" "$output/uke-boot-integration.spec"
mv "$inner" "$stage/inner/uke-boot-integration-1.8.14"
tar --sort=name --mtime=@1793509200 --owner=0 --group=0 --numeric-owner -cJf "$input/uke-boot-integration-1.8.14.tar.xz" -C "$stage/inner" uke-boot-integration-1.8.14
unlink -- "$input/uke-boot-integration-1.8.13.tar.xz"
commit=$(git -C "$owner" rev-parse HEAD)
jq -n --arg commit "$commit" '{source_commit:$commit,parent_build:11238010,parent_srpm_sha256:"f00ebb5a919664bd1b8baff32b8e4f6211b879e0f74d802bb19603be139e8652",functional_changes:["src/kernel-update.cpp"],retained_checks:["manifest","module bytes","RPM ownership","early firmware","physical slots","accepted baseline"],physical_acceptance:false}' > "$outer/boot-validation-overlay.json"
(cd "$outer"; find . -type f ! -name SOURCE-SHA256SUMS -print0 | sort -z | xargs -0 sha256sum > SOURCE-SHA256SUMS)
mv "$outer" "$stage/unpacked/uke-boot-integration-1.8.14"
tar --sort=name --mtime=@1793509200 --owner=0 --group=0 --numeric-owner -cJf "$output/uke-boot-integration-1.8.14.tar.xz" -C "$stage/unpacked" uke-boot-integration-1.8.14
test "$(rpmspec --target aarch64 --define 'dist %{nil}' -q --srpm --qf '%{VERSION}-%{RELEASE}' "$output/uke-boot-integration.spec")" = 1.8.14-1.fc46
