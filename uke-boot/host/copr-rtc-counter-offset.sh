#!/usr/bin/env bash
# COPR only: extend the qualified boot userspace without changing updater sources.
set -Eeuo pipefail
trap 'status=$?; printf "Counter-clock source failed at line %s (exit %s)\n" "$LINENO" "$status" >&2; exit "$status"' ERR
test ! -e /etc/senemos/kernel-update.conf
export LC_ALL=C
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
output=$(realpath -m "${1:?COPR source output directory}")
stage=$(mktemp -d)
trap 'find "$stage" -depth -delete' EXIT
mkdir -p "$output" "$stage/rpm" "$stage/outer" "$stage/inner"
curl -fLsS --max-time 45 --get --data-urlencode ownername=mcc45tr \
 --data-urlencode projectname=uke-linux-unstable --data-urlencode packagename=uke-boot-integration \
 --data-urlencode limit=100 https://copr.fedorainfracloud.org/api_3/build/list > "$stage/builds.json"
jq -e '.items|type=="array" and length>0' "$stage/builds.json" >/dev/null
# Other custom directories contain independent version experiments. Reject
# this exact NVR everywhere, and superseding releases in the intended cohort.
jq -r '.items[]|select(.source_package.name=="uke-boot-integration" and .source_package.url!=null)|
 select(.project_dirname=="uke-linux-unstable" or
 .project_dirname=="uke-linux-unstable:custom:rtc-counter-offset-20261105" or
 .source_package.version=="1.8.93-1.fc46")|.source_package.version' "$stage/builds.json" > "$stage/versions"
while IFS= read -r published; do
 [[ $published =~ ^[0-9]+([.][0-9]+)*-[0-9]+([.][a-zA-Z0-9]+)*$ ]]
 comparison=$(rpm --eval "%{lua:print(rpm.vercmp(\"$published\", \"1.8.93-1.fc46\"))}")
 [[ $comparison =~ ^-?[0-9]+$ && $comparison -lt 0 ]] || {
  printf 'Counter-clock candidate1.8.93 is already published or superseded; use a fresh version\n' >&2;exit 1;
 }
done < "$stage/versions"
curl -fLsS --retry 2 --max-time 90 --proto '=https' --proto-redir '=https' \
 https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/srpm-builds/11260223/uke-boot-integration-1.8.90-1.fc46.src.rpm \
 -o "$stage/parent.src.rpm"
printf '%s  %s\n' 07bcfb2598878b184f4f8df44a610f16fa653d56346fd328e14cd4b0bf9edc5c "$stage/parent.src.rpm" | sha256sum -c --strict
rpmkeys --checksig --nosignature --verbose "$stage/parent.src.rpm"
rpm2cpio "$stage/parent.src.rpm" > "$stage/parent.cpio"
cpio -t --quiet < "$stage/parent.cpio" > "$stage/entries"
awk '/^\/|(^|\/)\.\.($|\/)/ {bad=1} END {exit bad}' "$stage/entries"
(cd "$stage/rpm"; cpio -idm --quiet --no-absolute-filenames < "$stage/parent.cpio")
extract() {
 tar -tJf "$1" > "$stage/entries"
 awk '/^\/|(^|\/)\.\.($|\/)/ {bad=1} END {exit bad}' "$stage/entries"
 tar -xJf "$1" -C "$2"
}
extract "$stage/rpm/uke-boot-integration-1.8.90.tar.xz" "$stage/outer"
outer=$stage/outer/uke-boot-integration-1.8.90
extract "$outer/inputs/uke-boot-integration/uke-boot-integration-1.8.90.tar.xz" "$stage/inner"
native=$stage/inner/uke-boot-integration-1.8.90
# Pin the original tree before adding anything; whitelist only the two integrations.
(cd "$native"; find src tests -type f ! -path src/install-profile.cpp ! -path src/units/75-uke-clock-floor.conf -print0 | sort -z | xargs -0 sha256sum) > "$stage/retained.sha256"
[[ $(cat "$native/src/profiles/rollback.conf") == mode=enforced ]]
grep -Fxq '%config(noreplace) /etc/senemos/rollback.conf' "$stage/rpm/uke-boot-integration.spec"
git -C "$native" apply --check "$owner/patches/rtc-counter-clock-integration.patch"
git -C "$native" apply "$owner/patches/rtc-counter-clock-integration.patch"
for file in src/rtc-counter-offset.hpp tests/rtc-counter-offset.cpp tests/rtc-counter-offset.sh \
 src/units/uke-rtc-counter-sync.service src/units/uke-rtc-counter-sync.timer; do
 test ! -e "$native/$file"
 install -Dm644 "$owner/$file" "$native/$file"
done
install -Dm644 "$owner/docs/RTC-COUNTER-OFFSET.md" "$native/docs/RTC-COUNTER-OFFSET.md"
(cd "$native"; sha256sum -c --strict --quiet "$stage/retained.sha256")
printf 'All original updater, secure RTC, rollback/staging policy and test sources retained byte-for-byte\n'
spec=$output/uke-boot-integration.spec
awk '
 /^Source0:/ {print;print "BuildRequires: clang compiler-rt libasan libubsan";next}
 /^%check$/ {checking=1}
 checking && /^%[a-z]/ && $0!="%check" {
   print "bash %{_builddir}/uke-boot-integration-1.8.90/work/uke-boot-integration/uke-boot-integration-1.8.90/tests/rtc-counter-offset.sh"
   checking=0;checks++
 }
 /^%install$/ {installing=1}
 installing && /^%check$/ {
   print "install -Dm644 %{_builddir}/uke-boot-integration-1.8.90/work/uke-boot-integration/uke-boot-integration-1.8.90/src/units/uke-rtc-counter-sync.service %{buildroot}/usr/lib/systemd/system/uke-rtc-counter-sync.service"
   print "install -Dm644 %{_builddir}/uke-boot-integration-1.8.90/work/uke-boot-integration/uke-boot-integration-1.8.90/src/units/uke-rtc-counter-sync.timer %{buildroot}/usr/lib/systemd/system/uke-rtc-counter-sync.timer"
   installing=0;installs++
 }
 /^%files -n uke-installation-profile$/ {
   print;print "/usr/lib/systemd/system/uke-rtc-counter-sync.service";print "/usr/lib/systemd/system/uke-rtc-counter-sync.timer";files++;next
 }
 /^%package -n uke-installation-profile$/ {
   print;print "Provides: uke-installation-profile(rtc-counter-offset-v1) = 1";next
 }
 /^%transfiletriggerin -n uke-kernel-update -- / {
   if($0!="%transfiletriggerin -n uke-kernel-update -- /usr/lib/senemos")exit 1
   trigger++
 }
 {print}
 END {if(checks!=1 || installs!=1 || files!=1 || trigger!=1)exit 1}
' "$stage/rpm/uke-boot-integration.spec" | sed 's/1[.]8[.]90/1.8.93/g' > "$spec"
# Start only the new userspace timer, after the existing profile reconciliation.
awk '
 /^%posttrans -n uke-installation-profile$/ {section=1}
 section && /^%transfiletriggerin / {
   print "if [ -d /run/systemd/system ] && [ -d /sys/firmware/devicetree/base/soc/qcom,spmi@c42d000/qcom,pmk8550@0/rtc@6100 ] && [ ! -e /sys/firmware/devicetree/base/soc/qcom,spmi@c42d000/qcom,pmk8550@0/rtc@6100/qcom,uefi-rtc-info ]; then"
   print "    systemctl daemon-reload || :"
   print "    systemctl --no-block start uke-rtc-counter-sync.timer || :"
   print "fi";section=0;count++
 }
 {print}
 END {if(count!=1)exit 1}
' "$spec" > "$stage/final.spec"
mv "$stage/final.spec" "$spec"
awk '
 function entry() {
   print "* Thu Nov 05 2026 Uke contributors - 1.8.93-1"
   print "- Save trusted UTC with the read-only PMIC counter and restore elapsed time at startup."
   print "- Retain original secure RTC, seamless staging and persistent rollback preferences."
   print ""
 }
 /^%changelog$/ {print;entry();found=1;next}
 {print}
 END {if(!found){print "%changelog";entry()}}
' "$spec" > "$stage/changelog.spec"
mv "$stage/changelog.spec" "$spec"
mv "$native" "$stage/inner/uke-boot-integration-1.8.93"
epoch=$(git -C "$owner" show -s --format=%ct HEAD)
rm "$outer/inputs/uke-boot-integration/uke-boot-integration-1.8.90.tar.xz"
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf \
 "$outer/inputs/uke-boot-integration/uke-boot-integration-1.8.93.tar.xz" -C "$stage/inner" uke-boot-integration-1.8.93
cp "$spec" "$outer/inputs/uke-boot-integration/uke-boot-integration.spec"
jq -n --arg source "$(git -C "$owner" rev-parse HEAD)" \
 '{schema_version:1,parent_build:11260223,parent_srpm_sha256:"07bcfb2598878b184f4f8df44a610f16fa653d56346fd328e14cd4b0bf9edc5c",
 source_commit:$source,backend:"private-ntp-anchor-plus-readonly-pmic-counter",pmic_writes:false,oem_persist_writes:false,
 retained_native_sources_unchanged_except:["src/install-profile.cpp","src/units/75-uke-clock-floor.conf"],
 automatic_staging_preserved:true,rollback_default:"enforced",physical_acceptance:false}' > "$outer/rtc-counter-offset-overlay.json"
mv "$outer" "$stage/outer/uke-boot-integration-1.8.93"
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf \
 "$output/uke-boot-integration-1.8.93.tar.xz" -C "$stage/outer" uke-boot-integration-1.8.93
candidate=$(rpmspec --define 'dist %{nil}' -q --srpm --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$spec")
printf 'Counter-clock source candidate: %s\n' "$candidate"
[[ $candidate == uke-boot-integration-1.8.93-1.fc46 ]]
