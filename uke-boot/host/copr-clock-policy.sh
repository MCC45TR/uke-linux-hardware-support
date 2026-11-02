#!/bin/bash
# Run only in COPR: retain the installed boot cohort and overlay checked repairs.
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
        all(.items[]; ((.source_package.version // "")|sub("[.]fc[0-9]+([.][0-9]+)*$";""))!="1.8.33-1")' "$stage/builds.json" >/dev/null
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
for relative in src/touch-health.hpp src/shutdown-trace.sh; do
    cmp "$inner/$relative" "$owner/$relative"
done
for relative in src/units/70-uke-chronyd-clock.conf src/units/75-uke-clock-floor.conf src/units/uke-userdata-accept.service \
    src/profiles/clock-epoch src/profiles/uke-system-heap.conf src/udev/72-uke-system-heap.rules \
    src/kernel-update.cpp src/install-profile.cpp src/userdata-update.hpp src/slot-control.hpp src/oem-video.hpp \
    tests/kernel-update.cpp tests/install-profile.cpp tests/userdata-update.cpp tests/slot-control.cpp tests/oem-video.cpp tests/oem-video.sh \
    docs/KERNEL-UPDATES.md docs/USERDATA-AB.md; do
    install -Dm644 "$owner/$relative" "$inner/$relative"
done
rewrite_spec() {
    awk '/^%changelog/ {print;print "* Mon Nov 02 2026 MCC45TR <75160848+MCC45TR@users.noreply.github.com> - 1.8.33-1";print "- Add bounded device-local video import and original Fedora integration dependencies.";print "";history=1;next}
        !history {gsub(/1[.]8[.]24-1/,"1.8.33-1");gsub(/1[.]8[.]24/,"1.8.33")}
        {print}
        /^BuildRequires: jq$/ {print "BuildRequires: e2fsprogs"}
        /^Requires: bluez$/ {
            print "Requires: linux-firmware"
            print "Requires: (libcamera-ipa if wireplumber)"
            print "Requires: (pipewire-plugin-libcamera if wireplumber)"}
        /^Requires: systemd >= 254$/ {print "Requires: e2fsprogs"}
        /^Provides: uke-kernel-update\(qualified-firmware-cohort-v1\) = 1$/ {
            print "Provides: uke-kernel-update(inactive-slot-no-image-backup-v1) = 1"
            print "Provides: uke-kernel-update(device-local-video-import-v1) = 1"}
        /^install -Dm644 src\/units\/70-uke-chronyd-clock[.]conf / {
            print "install -Dm644 src/profiles/uke-system-heap.conf %{buildroot}%{_prefix}/lib/modules-load.d/uke-system-heap.conf"
            print "install -Dm644 src/udev/72-uke-system-heap.rules %{buildroot}%{_udevrulesdir}/72-uke-system-heap.rules"
            print "install -pDm644 src/profiles/clock-epoch %{buildroot}%{_prefix}/lib/clock-epoch"}
        /^install -Dm755 uke-install-profile / {
            print "install -Dm644 src/units/75-uke-clock-floor.conf %{buildroot}%{_unitdir}/chronyd.service.d/75-uke-clock-floor.conf"}
        /^%\{_libexecdir\}\/senemos-uke\/uke-install-profile$/ {
            print "%{_unitdir}/chronyd.service.d/75-uke-clock-floor.conf"}
        /^%\{_unitdir\}\/chronyd[.]service[.]d\/70-uke-clock[.]conf$/ {
            print "%{_prefix}/lib/modules-load.d/uke-system-heap.conf"
            print "%{_udevrulesdir}/72-uke-system-heap.rules"
            print "%{_prefix}/lib/clock-epoch"}
        /^%check$/ {
            print "test -f %{buildroot}%{_prefix}/lib/clock-epoch"
            print "test $(stat -c %Y %{buildroot}%{_prefix}/lib/clock-epoch) -eq $(stat -c %Y src/profiles/clock-epoch)"
            print "bash tests/oem-video.sh"
            print "udevadm verify src/udev/72-uke-system-heap.rules"
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
    awk '/^%changelog/ {print;print "* Mon Nov 02 2026 MCC45TR <75160848+MCC45TR@users.noreply.github.com> - 1.8.33-1";print "- Add bounded device-local video import and original Fedora integration dependencies.";print "";history=1;next}
        !history {gsub(/1[.]8[.]24-1/,"1.8.33-1");gsub(/1[.]8[.]24/,"1.8.33")}
        /^%check$/ {checking=1}
        {print}
        /^BuildRequires: jq$/ {print "BuildRequires: e2fsprogs"}
        /^Requires: bluez$/ {
            print "Requires: linux-firmware"
            print "Requires: (libcamera-ipa if wireplumber)"
            print "Requires: (pipewire-plugin-libcamera if wireplumber)"}
        /^Requires: systemd >= 254$/ {print "Requires: e2fsprogs"}
        /^Provides: uke-kernel-update\(qualified-firmware-cohort-v1\) = 1$/ {
            print "Provides: uke-kernel-update(inactive-slot-no-image-backup-v1) = 1"
            print "Provides: uke-kernel-update(device-local-video-import-v1) = 1"}
        /^install -Dm644 src\/units\/70-uke-chronyd-clock[.]conf / {
            print "install -Dm644 src/profiles/uke-system-heap.conf %{buildroot}/usr/lib/modules-load.d/uke-system-heap.conf"
            print "install -Dm644 src/udev/72-uke-system-heap.rules %{buildroot}/usr/lib/udev/rules.d/72-uke-system-heap.rules"
            print "install -pDm644 src/profiles/clock-epoch %{buildroot}/usr/lib/clock-epoch"}
        /^install -Dm755 uke-install-profile / {
            print "install -Dm644 src/units/75-uke-clock-floor.conf %{buildroot}/usr/lib/systemd/system/chronyd.service.d/75-uke-clock-floor.conf"}
        /^\/usr\/libexec\/senemos-uke\/uke-install-profile$/ {
            print "/usr/lib/systemd/system/chronyd.service.d/75-uke-clock-floor.conf"}
        /^\/usr\/lib\/systemd\/system\/chronyd[.]service[.]d\/70-uke-clock[.]conf$/ {
            print "/usr/lib/modules-load.d/uke-system-heap.conf"
            print "/usr/lib/udev/rules.d/72-uke-system-heap.rules"
            print "/usr/lib/clock-epoch"}
        checking && /^cd / {
            print "test -f %{buildroot}/usr/lib/clock-epoch"
            print "test $(stat -c %Y %{buildroot}/usr/lib/clock-epoch) -eq $(stat -c %Y src/profiles/clock-epoch)"
            print "bash tests/oem-video.sh"
            print "udevadm verify src/udev/72-uke-system-heap.rules"
            checking=0
        }' "$1" > "$2"
}
rewrite_expanded_spec "$stage/sources/uke-boot-integration.spec" "$output/uke-boot-integration.spec"
test "$(grep -Fxc '/usr/lib/clock-epoch' "$output/uke-boot-integration.spec")" = 1
test "$(grep -Fxc 'install -pDm644 src/profiles/clock-epoch %{buildroot}/usr/lib/clock-epoch' "$output/uke-boot-integration.spec")" = 1
test "$(grep -Fxc 'test -f %{buildroot}/usr/lib/clock-epoch' "$output/uke-boot-integration.spec")" = 1
test "$(grep -Fxc 'bash tests/oem-video.sh' "$output/uke-boot-integration.spec")" = 1
test "$(grep -Fxc 'Provides: uke-kernel-update(device-local-video-import-v1) = 1' "$output/uke-boot-integration.spec")" = 1
test "$(grep -Fxc 'Requires: linux-firmware' "$output/uke-boot-integration.spec")" = 1
test "$(grep -Fxc 'Requires: (pipewire-plugin-libcamera if wireplumber)' "$output/uke-boot-integration.spec")" = 1
test "$(grep -Fxc '/usr/lib/modules-load.d/uke-system-heap.conf' "$output/uke-boot-integration.spec")" = 1
test "$(grep -Fxc '/usr/lib/udev/rules.d/72-uke-system-heap.rules' "$output/uke-boot-integration.spec")" = 1
test "$(grep -Fxc 'Requires: (libcamera-ipa if wireplumber)' "$output/uke-boot-integration.spec")" = 1
test "$(grep -Fxc '/usr/lib/systemd/system/chronyd.service.d/75-uke-clock-floor.conf' "$output/uke-boot-integration.spec")" = 1
commit=$(git -C "$owner" rev-parse HEAD)
epoch=$(git -C "$owner" show -s --format=%ct HEAD)
mv "$inner" "$stage/inner/uke-boot-integration-1.8.33"
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf "$input/uke-boot-integration-1.8.33.tar.xz" -C "$stage/inner" uke-boot-integration-1.8.33
unlink "$input/uke-boot-integration-1.8.24.tar.xz"
jq -n --arg commit "$commit" --argjson epoch "$epoch" \
    '{source_commit:$commit,parent_build:11240620,parent_srpm_sha256:"3a8e8923d0b127b13e7b2f4f819ba0da25e2bc7dc0f55ec64393ed561d7118a9",clock_epoch:$epoch,whole_boot_image_copies:false,inactive_unbootable_barrier:true,device_local_video_import:true,firmware_payload_included:false,rtc_writes:false,automatic_policy_unchanged:true,physical_acceptance:false}' > "$outer/clock-policy-overlay.json"
(cd "$outer"; find . -type f ! -name SOURCE-SHA256SUMS -print0 | sort -z | xargs -0 sha256sum > SOURCE-SHA256SUMS)
mv "$outer" "$stage/unpacked/uke-boot-integration-1.8.33"
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf "$output/uke-boot-integration-1.8.33.tar.xz" -C "$stage/unpacked" uke-boot-integration-1.8.33
test "$(rpmspec --target aarch64 --define 'dist %{nil}' -q --srpm --qf '%{VERSION}-%{RELEASE}' "$output/uke-boot-integration.spec")" = 1.8.33-1.fc46
