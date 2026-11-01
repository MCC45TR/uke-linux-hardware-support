#!/usr/bin/env bash
# COPR source worker: retain the complete qualified boot source, change RPM metadata only.
set -Eeuo pipefail
[[ ${UKE_COPR_GNU_RETURN:-} == 1 ]]
[[ $(rpm --eval '%{_host_cpu}') == aarch64 && $(rpm --eval '%{fedora}') == 46 ]]
test ! -e /etc/senemos/kernel-update.conf
root=$(cd -- "$(dirname -- "$0")/../../..";pwd)
output=$(realpath -m "${1:?result directory}")
stage=$(mktemp -d)
trap 'rm -rf -- "$stage"' EXIT
mkdir -p "$stage/sources" "$stage/outer" "$stage/inner" "$output"
url=https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/srpm-builds/11239644/uke-boot-integration-1.8.18-1.fc46.src.rpm
curl -fLsS --proto '=https' --proto-redir '=https' --max-time 120 "$url" -o "$stage/base.src.rpm"
printf '%s  %s\n' fc3aa33e9f07be4d6f4dc0677034e1fc13fc5ad41c0c3894e524b6e309f3ee28 "$stage/base.src.rpm" | sha256sum -c -
rpm2cpio "$stage/base.src.rpm" > "$stage/source.cpio"
cpio -t --quiet < "$stage/source.cpio" > "$stage/members"
awk '/^\/|(^|\/)\.\.($|\/)/ {bad=1} END {exit bad}' "$stage/members"
(cd "$stage/sources";cpio -idm --quiet --no-absolute-filenames < "$stage/source.cpio")
tar -xJf "$stage/sources/uke-boot-integration-1.8.18.tar.xz" -C "$stage/outer"
outer=$stage/outer/uke-boot-integration-1.8.18
(cd "$outer";sha256sum --check --strict --quiet SOURCE-SHA256SUMS)
input=$outer/inputs/uke-boot-integration
tar -xJf "$input/uke-boot-integration-1.8.18.tar.xz" -C "$stage/inner"
inner=$stage/inner/uke-boot-integration-1.8.18
source_hashes() { (cd "$inner";find . -type f ! -path './packaging/uke-boot-integration.spec' -print0 | LC_ALL=C sort -z | xargs -0 sha256sum); }
source_hashes > "$stage/programs.before"
for spec in "$inner/packaging/uke-boot-integration.spec" "$input/uke-boot-integration.spec" "$stage/sources/uke-boot-integration.spec";do
    bash "$root/packaging/families/host/fedora-cxx-runtime.sh" "$spec" "$stage/runtime.spec"
    awk '
      /^Release:/ {if($2!~/^1(%|[.]fc46|$)/)exit 1;sub(/^1/,"2",$2)}
      /^%changelog/ {print;print "* Sun Nov 01 2026 Uke Maintainers <75160848+MCC45TR@users.noreply.github.com> - 1.8.18-2";print "- Use the original Fedora GNU C++ runtime; retain all boot programs and service behavior.";print "";history=1;next}
      !history {gsub(/1[.]8[.]18-1[.]fc46/,"1.8.18-2.fc46")}
      {print}
    ' "$stage/runtime.spec" > "$stage/released.spec"
    install -m644 "$stage/released.spec" "$spec"
done
source_hashes > "$stage/programs.after"
cmp "$stage/programs.before" "$stage/programs.after"
epoch=$(git -C "$root" show -s --format=%ct HEAD)
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf "$input/uke-boot-integration-1.8.18.tar.xz" -C "$stage/inner" uke-boot-integration-1.8.18
jq -n '{date:"2026-11-01",parent_build:11239644,parent_srpm_sha256:"fc3aa33e9f07be4d6f4dc0677034e1fc13fc5ad41c0c3894e524b6e309f3ee28",changes:"GNU runtime Requires and monotonic RPM Release only",program_source_hashes_preserved:true,hardware_tested:false}' > "$outer/fedora-cxx-overlay.json"
(cd "$outer";find . -type f ! -name SOURCE-SHA256SUMS -print0 | LC_ALL=C sort -z | xargs -0 sha256sum > SOURCE-SHA256SUMS)
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf "$output/uke-boot-integration-1.8.18.tar.xz" -C "$stage/outer" uke-boot-integration-1.8.18
install -m644 "$stage/sources/uke-boot-integration.spec" "$output/uke-boot-integration.spec"
test "$(rpmspec --target aarch64 -q --srpm --qf '%{VERSION}-%{RELEASE}' "$output/uke-boot-integration.spec")" = 1.8.18-2.fc46
echo 'UKE_GNU_BOOT_SOURCE parent=11239644 program_sources_unchanged=true requires_updated=4 rpm_release=2'
