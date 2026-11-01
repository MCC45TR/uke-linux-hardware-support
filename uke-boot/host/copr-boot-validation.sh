#!/bin/bash
# COPR-only: retain the verified boot family and complete the empty clock default.
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
 https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/srpm-builds/11239526/uke-boot-integration-1.8.15-1.fc46.src.rpm -o "$stage/base.src.rpm"
printf '%s  %s\n' e27644494d77e6c8da367a33102b573ec0e672a23b8b8ce3e4e4c23545f67506 "$stage/base.src.rpm" | sha256sum -c -
rpm2cpio "$stage/base.src.rpm" > "$stage/base.cpio"
cpio -t --quiet < "$stage/base.cpio" | awk '/^\/|(^|\/)\.\.($|\/)/ {bad=1} END {exit bad}'
mkdir -p "$stage/sources" "$stage/unpacked" "$stage/inner" "$output"
(cd "$stage/sources"; cpio -idm --quiet --no-absolute-filenames < "$stage/base.cpio")
tar -xJf "$stage/sources/uke-boot-integration-1.8.15.tar.xz" -C "$stage/unpacked"
outer=$stage/unpacked/uke-boot-integration-1.8.15
(cd "$outer"; sha256sum --check --strict --quiet SOURCE-SHA256SUMS)
input=$outer/inputs/uke-boot-integration
tar -xJf "$input/uke-boot-integration-1.8.15.tar.xz" -C "$stage/inner"
inner=$stage/inner/uke-boot-integration-1.8.15
for relative in src/kernel-update.cpp src/userdata-update.hpp src/installation-policy.hpp src/units/uke-userdata-accept.service; do
 cmp "$inner/$relative" "$owner/$relative"
done
for relative in src/shutdown-trace.sh src/units/uke-shutdown-trace.service tests/shutdown-trace.sh docs/SHUTDOWN-TRACE.md src/install-profile.cpp src/units/uke-install-profile.service tests/install-profile.cpp tests/installation-policy.sh docs/USERDATA-AB.md; do
 install -m644 "$owner/$relative" "$inner/$relative"
done
rewrite_spec() {
 awk '/^%changelog/ {print;print "* Sun Nov 01 2026 MCC45TR <75160848+MCC45TR@users.noreply.github.com> - 1.8.17-1";print "- Complete only the empty Fedora clock default with UTC without RTC access.";print "- Retain bounded USB tracing and existing updater/slot validation.";print "";history=1;next}
      !history {
        gsub(/1[.]8[.]15/,"1.8.17")
        if(/^install -Dm644 src\/units\/30-uke-plasmalogin-ready.conf /) {
          print;print "install -Dm755 src/shutdown-trace.sh %{buildroot}%{_libexecdir}/senemos-uke/uke-shutdown-trace";print "install -Dm644 src/units/uke-shutdown-trace.service %{buildroot}%{_unitdir}/uke-shutdown-trace.service";next
        }
        if(!/^install / && /\/plasmalogin[.]service[.]d\/30-uke-native-drm[.]conf$/) {
          print;print "%{_libexecdir}/senemos-uke/uke-shutdown-trace";print "%{_unitdir}/uke-shutdown-trace.service";next
        }
        if(/^cmp src\/profiles\/60-uke-security.conf /) {print;print "bash tests/shutdown-trace.sh";next}
        if(/^%doc / && /TOUCH-RUNTIME[.]md$/) {line=$0;sub(/TOUCH-RUNTIME[.]md$/,"SHUTDOWN-TRACE.md",line);print;print line;next}
        if(/^%posttrans( -n uke-boot-integration)?$/) {
          print;print "# Temporary current-boot diagnosis, without boot enablement or a reboot.";print "if [ -d /run/systemd/system ]; then";print "    systemctl --no-block start uke-shutdown-trace.service || :";print "fi";next
        }
        if(/^%preun( -n uke-boot-integration)?$/) {
          print;printf "if [ %c$1%c -eq 0 ] && [ -d /run/systemd/system ]; then\n",34,34;print "    systemctl stop uke-shutdown-trace.service || :";print "fi";next
        }
      } {print}' "$1" > "$2"
 test "$(grep -c '^install -Dm755 src/shutdown-trace.sh ' "$2")" = 1
 test "$(grep -c '^%{_unitdir}/uke-shutdown-trace.service$' "$2")" = 1
 test "$(grep -c '^bash tests/shutdown-trace.sh$' "$2")" = 1
 test "$(grep -c 'systemctl --no-block start uke-shutdown-trace.service' "$2")" = 1
 test "$(grep -c '^%doc .*docs/SHUTDOWN-TRACE.md$' "$2")" = 1
}
rewrite_spec "$inner/packaging/uke-boot-integration.spec" "$stage/native.spec"
install -m644 "$stage/native.spec" "$inner/packaging/uke-boot-integration.spec"
rewrite_spec "$input/uke-boot-integration.spec" "$stage/input.spec"
install -m644 "$stage/input.spec" "$input/uke-boot-integration.spec"
rewrite_spec "$stage/sources/uke-boot-integration.spec" "$output/uke-boot-integration.spec"
commit=$(git -C "$owner" rev-parse HEAD)
epoch=$(git -C "$owner" show -s --format=%ct HEAD)
mv "$inner" "$stage/inner/uke-boot-integration-1.8.17"
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf "$input/uke-boot-integration-1.8.17.tar.xz" -C "$stage/inner" uke-boot-integration-1.8.17
unlink -- "$input/uke-boot-integration-1.8.15.tar.xz"
jq -n --arg commit "$commit" '{source_commit:$commit,parent_build:11239526,parent_srpm_sha256:"e27644494d77e6c8da367a33102b573ec0e672a23b8b8ce3e4e4c23545f67506",functional_changes:["src/shutdown-trace.sh","src/units/uke-shutdown-trace.service","src/install-profile.cpp","src/units/uke-install-profile.service"],native_updater_unchanged:true,trace_instance:"uke_shutdown",stack_limit_per_probe:32,usb_control_writes:false,boot_enablement:false,rtc_access:false,physical_acceptance:false}' > "$outer/shutdown-trace-overlay.json"
(cd "$outer"; find . -type f ! -name SOURCE-SHA256SUMS -print0 | sort -z | xargs -0 sha256sum > SOURCE-SHA256SUMS)
mv "$outer" "$stage/unpacked/uke-boot-integration-1.8.17"
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf "$output/uke-boot-integration-1.8.17.tar.xz" -C "$stage/unpacked" uke-boot-integration-1.8.17
test "$(rpmspec --target aarch64 --define 'dist %{nil}' -q --srpm --qf '%{VERSION}-%{RELEASE}' "$output/uke-boot-integration.spec")" = 1.8.17-1.fc46
