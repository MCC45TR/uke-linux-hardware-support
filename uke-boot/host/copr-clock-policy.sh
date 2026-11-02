#!/bin/bash
# Run only in COPR: retain the installed boot cohort and overlay clock/policy fixes.
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
    curl --fail --silent --show-error --max-time 45 --get --data-urlencode ownername=mcc45tr \
        --data-urlencode "projectname=$project" --data-urlencode packagename=uke-boot-integration \
        --data-urlencode limit=100 https://copr.fedorainfracloud.org/api_3/build/list > "$stage/builds.json"
    jq -e '(.items|type)=="array" and (.items|length)<100 and
        all(.items[]; ((.source_package.version // "")|sub("[.]fc[0-9]+([.][0-9]+)*$";""))!="1.8.27-1")' "$stage/builds.json" >/dev/null
done
curl -fLsS --proto '=https' --proto-redir '=https' --retry 3 \
    https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/srpm-builds/11240620/uke-boot-integration-1.8.24-1.fc46.src.rpm -o "$stage/base.src.rpm"
printf '%s  %s\n' 3a8e8923d0b127b13e7b2f4f819ba0da25e2bc7dc0f55ec64393ed561d7118a9 "$stage/base.src.rpm" | sha256sum -c -
rpm2cpio "$stage/base.src.rpm" > "$stage/base.cpio"
cpio -t --quiet < "$stage/base.cpio" | awk '/^\/|(^|\/)\.\.($|\/)/ {bad=1} END {exit bad}'
mkdir -p "$stage/sources" "$stage/unpacked" "$stage/inner" "$output"
(cd "$stage/sources"; cpio -idm --quiet --no-absolute-filenames < "$stage/base.cpio")
tar -tJf "$stage/sources/uke-boot-integration-1.8.24.tar.xz" | awk '/^\/|(^|\/)\.\.($|\/)/ {bad=1} END {exit bad}'
tar -xJf "$stage/sources/uke-boot-integration-1.8.24.tar.xz" -C "$stage/unpacked"
outer=$stage/unpacked/uke-boot-integration-1.8.24
(cd "$outer"; sha256sum --check --strict --quiet SOURCE-SHA256SUMS)
input=$outer/inputs/uke-boot-integration
tar -tJf "$input/uke-boot-integration-1.8.24.tar.xz" | awk '/^\/|(^|\/)\.\.($|\/)/ {bad=1} END {exit bad}'
tar -xJf "$input/uke-boot-integration-1.8.24.tar.xz" -C "$stage/inner"
inner=$stage/inner/uke-boot-integration-1.8.24
# All non-overlaid boot/runtime sources remain byte-identical to the parent.
for relative in src/install-profile.cpp src/touch-health.hpp src/shutdown-trace.sh; do
    cmp "$inner/$relative" "$owner/$relative"
done
for relative in src/units/70-uke-chronyd-clock.conf src/units/uke-userdata-accept.service \
    src/profiles/clock-epoch src/kernel-update.cpp src/userdata-update.hpp \
    tests/kernel-update.cpp tests/userdata-update.cpp docs/KERNEL-UPDATES.md docs/USERDATA-AB.md; do
    install -Dm644 "$owner/$relative" "$inner/$relative"
done
rewrite_spec() {
    awk '/^%changelog/ {print;print "* Mon Nov 02 2026 MCC45TR <75160848+MCC45TR@users.noreply.github.com> - 1.8.27-1";print "- Preserve systemd clock epoch and accept disabled legacy userdata policy.";print "";history=1;next}
        !history {gsub(/1[.]8[.]24-1/,"1.8.27-1");gsub(/1[.]8[.]24/,"1.8.27")}
        {print}
        /^Provides: uke-kernel-update\(qualified-firmware-cohort-v1\) = 1$/ {
            print "Provides: uke-kernel-update(inactive-slot-no-image-backup-v1) = 1"}
        /^install -Dm644 src\/units\/70-uke-chronyd-clock[.]conf / {
            print "install -pDm644 src/profiles/clock-epoch %{buildroot}%{_prefix}/lib/clock-epoch"}
        /^%\{_unitdir\}\/chronyd[.]service[.]d\/70-uke-clock[.]conf$/ {
            print "%{_prefix}/lib/clock-epoch"}
        /^%check$/ {
            print "test -f %{buildroot}%{_prefix}/lib/clock-epoch"
            print "test $(stat -c %Y %{buildroot}%{_prefix}/lib/clock-epoch) -eq $(stat -c %Y src/profiles/clock-epoch)"
        }' "$1" > "$2"
}
for spec in "$inner/packaging/uke-boot-integration.spec" "$input/uke-boot-integration.spec"; do
    rewrite_spec "$spec" "$stage/updated.spec"
    test "$(grep -Fxc '%{_prefix}/lib/clock-epoch' "$stage/updated.spec")" = 1
    install -m644 "$stage/updated.spec" "$spec"
done
rewrite_expanded_spec() {
    # The standalone outer spec contains the actual expanded install/check/file
    # sections. Editing its retained inner recipe alone does not change the RPM.
    awk '/^%changelog/ {print;print "* Mon Nov 02 2026 MCC45TR <75160848+MCC45TR@users.noreply.github.com> - 1.8.27-1";print "- Install and check the systemd epoch in the expanded standalone recipe.";print "";history=1;next}
        !history {gsub(/1[.]8[.]24-1/,"1.8.27-1");gsub(/1[.]8[.]24/,"1.8.27")}
        /^%check$/ {checking=1}
        {print}
        /^Provides: uke-kernel-update\(qualified-firmware-cohort-v1\) = 1$/ {
            print "Provides: uke-kernel-update(inactive-slot-no-image-backup-v1) = 1"}
        /^install -Dm644 src\/units\/70-uke-chronyd-clock[.]conf / {
            print "install -pDm644 src/profiles/clock-epoch %{buildroot}/usr/lib/clock-epoch"}
        /^\/usr\/lib\/systemd\/system\/chronyd[.]service[.]d\/70-uke-clock[.]conf$/ {
            print "/usr/lib/clock-epoch"}
        checking && /^cd / {
            print "test -f %{buildroot}/usr/lib/clock-epoch"
            print "test $(stat -c %Y %{buildroot}/usr/lib/clock-epoch) -eq $(stat -c %Y src/profiles/clock-epoch)"
            checking=0
        }' "$1" > "$2"
}
rewrite_expanded_spec "$stage/sources/uke-boot-integration.spec" "$output/uke-boot-integration.spec"
test "$(grep -Fxc '/usr/lib/clock-epoch' "$output/uke-boot-integration.spec")" = 1
test "$(grep -Fxc 'install -pDm644 src/profiles/clock-epoch %{buildroot}/usr/lib/clock-epoch' "$output/uke-boot-integration.spec")" = 1
test "$(grep -Fxc 'test -f %{buildroot}/usr/lib/clock-epoch' "$output/uke-boot-integration.spec")" = 1
commit=$(git -C "$owner" rev-parse HEAD)
epoch=$(git -C "$owner" show -s --format=%ct HEAD)
mv "$inner" "$stage/inner/uke-boot-integration-1.8.27"
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf "$input/uke-boot-integration-1.8.27.tar.xz" -C "$stage/inner" uke-boot-integration-1.8.27
unlink "$input/uke-boot-integration-1.8.24.tar.xz"
jq -n --arg commit "$commit" --argjson epoch "$epoch" \
    '{source_commit:$commit,parent_build:11240620,parent_srpm_sha256:"3a8e8923d0b127b13e7b2f4f819ba0da25e2bc7dc0f55ec64393ed561d7118a9",clock_epoch:$epoch,rtc_writes:false,automatic_policy_unchanged:true,physical_acceptance:false}' > "$outer/clock-policy-overlay.json"
(cd "$outer"; find . -type f ! -name SOURCE-SHA256SUMS -print0 | sort -z | xargs -0 sha256sum > SOURCE-SHA256SUMS)
mv "$outer" "$stage/unpacked/uke-boot-integration-1.8.27"
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf "$output/uke-boot-integration-1.8.27.tar.xz" -C "$stage/unpacked" uke-boot-integration-1.8.27
test "$(rpmspec --target aarch64 --define 'dist %{nil}' -q --srpm --qf '%{VERSION}-%{RELEASE}' "$output/uke-boot-integration.spec")" = 1.8.27-1.fc46
