#!/usr/bin/env bash
# COPR-only: retain the published boot family and fix updater mountinfo reads.
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
 https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/srpm-builds/11233005/uke-boot-integration-1.8.11-1.fc46.src.rpm -o "$stage/base.src.rpm"
printf '%s  %s\n' 829c357639fdf912b333bbbc161b19871ace5b1cad1de3eab3d490570011cbd9 "$stage/base.src.rpm" | sha256sum -c -
rpm2cpio "$stage/base.src.rpm" > "$stage/base.cpio"
cpio -t --quiet < "$stage/base.cpio" | awk '/^\/|(^|\/)\.\.($|\/)/ {bad=1} END {exit bad}'
mkdir -p "$stage/sources" "$stage/unpacked" "$stage/inner" "$output"
(cd "$stage/sources"; cpio -idm --quiet --no-absolute-filenames < "$stage/base.cpio")
printf '%s  %s\n' 671e3c3f9bca9c79af4fee07e5262d2940627f605801883cd8cd018a43c71d1d "$stage/sources/uke-boot-integration-1.8.11.tar.xz" | sha256sum -c -
tar -xJf "$stage/sources/uke-boot-integration-1.8.11.tar.xz" -C "$stage/unpacked"
outer=$stage/unpacked/uke-boot-integration-1.8.11
(cd "$outer"; sha256sum --check --strict --quiet SOURCE-SHA256SUMS)
input=$outer/inputs/uke-boot-integration
printf '%s  %s\n' 22f952e343a4efdc8aca7a95f645936264e43e3eb345ab8ea48197023a1f8284 "$input/uke-boot-integration-1.8.11.tar.xz" | sha256sum -c -
tar -xJf "$input/uke-boot-integration-1.8.11.tar.xz" -C "$stage/inner"
inner=$stage/inner/uke-boot-integration-1.8.11
for relative in src/kernel-update.cpp tests/kernel-update.cpp docs/KERNEL-UPDATES.md; do
    install -m644 "$owner/$relative" "$inner/$relative"
done
# Change active package/path references while retaining historical changelogs.
rewrite_spec() {
 awk '/^%changelog/ {print; print "* Sat Oct 31 2026 MCC45TR <75160848+MCC45TR@users.noreply.github.com> - 1.8.12-1"; print "- Read complete bounded mount information before native update admission."; print ""; history=1; next}
      !history {gsub(/1[.]8[.]11/,"1.8.12")} {print}' "$1" > "$2"
}
rewrite_spec "$inner/packaging/uke-boot-integration.spec" "$stage/native.spec"
install -m644 "$stage/native.spec" "$inner/packaging/uke-boot-integration.spec"
rewrite_spec "$input/uke-boot-integration.spec" "$stage/input.spec"
install -m644 "$stage/input.spec" "$input/uke-boot-integration.spec"
rewrite_spec "$stage/sources/uke-boot-integration.spec" "$output/uke-boot-integration.spec"
mv "$inner" "$stage/inner/uke-boot-integration-1.8.12"
tar --sort=name --mtime=@1793422800 --owner=0 --group=0 --numeric-owner -cJf "$input/uke-boot-integration-1.8.12.tar.xz" -C "$stage/inner" uke-boot-integration-1.8.12
unlink -- "$input/uke-boot-integration-1.8.11.tar.xz"
commit=$(git -C "$owner" rev-parse HEAD)
jq -n --arg commit "$commit" '{source_commit:$commit,parent_build:11233005,parent_srpm_sha256:"829c357639fdf912b333bbbc161b19871ace5b1cad1de3eab3d490570011cbd9",functional_changes:["src/kernel-update.cpp","tests/kernel-update.cpp"],metadata_changes:["package version","docs/KERNEL-UPDATES.md"],physical_acceptance:false}' > "$outer/mountinfo-source-overlay.json"
printf '\n31 October 2026: derived boot family 1.8.12. Retained parent11233005 with an explicit native mountinfo reader/test overlay and package-version metadata. All compilation and tests run on COPR; hardware admission is separate.\n' >> "$outer/README"
(cd "$outer"; find . -type f ! -name SOURCE-SHA256SUMS -print0 | sort -z | xargs -0 sha256sum > SOURCE-SHA256SUMS)
mv "$outer" "$stage/unpacked/uke-boot-integration-1.8.12"
tar --sort=name --mtime=@1793422800 --owner=0 --group=0 --numeric-owner -cJf "$output/uke-boot-integration-1.8.12.tar.xz" -C "$stage/unpacked" uke-boot-integration-1.8.12
test "$(rpmspec --target aarch64 --define 'dist %{nil}' -q --srpm --qf '%{VERSION}-%{RELEASE}' "$output/uke-boot-integration.spec")" = 1.8.12-1.fc46
