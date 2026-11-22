#!/usr/bin/bash
# COPR only: fold the actual existing helper into the accepted Control cohort.
set -Eeuo pipefail
test ! -e /etc/senemos/kernel-update.conf
[[ $# == 1 ]]
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
workspace=$(dirname "$owner")
manifest=$owner/packaging/charge-fold-source.json
output=$(realpath -m "$1")
[[ ! -e $output ]]
(cd "$workspace";jq -r '.source_assets[]|.sha256+"  "+.path' "$manifest" | sha256sum -c --strict --quiet -)
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
for parent in control helper;do
 curl -qfsSL --max-time 120 "$(jq -er ".parents.$parent.url" "$manifest")" -o "$work/$parent.rpm"
 printf '%s  %s\n' "$(jq -er ".parents.$parent.sha256" "$manifest")" "$work/$parent.rpm" | sha256sum -c --strict --quiet -
 rpm2cpio "$work/$parent.rpm" > "$work/$parent.cpio"
 cpio -t --quiet < "$work/$parent.cpio" | awk '/^\/|(^|\/)\.\.($|\/)/{bad=1}END{exit bad}'
done
mkdir "$output" "$work/helper-parent" "$work/helper" "$work/overlay"
(cd "$output";cpio -idm --quiet --no-absolute-filenames < "$work/control.cpio")
(cd "$work/helper-parent";cpio -idm --quiet --no-absolute-filenames < "$work/helper.cpio")
printf '%s  %s\n' "$(jq -er .parents.control.spec_sha256 "$manifest")" "$output/uke-control.spec" | sha256sum -c --strict --quiet -
tar -tJf "$work/helper-parent/uke-battery-observer-1.4.1.tar.xz" | awk '/^\/|(^|\/)\.\.($|\/)/{bad=1}END{exit bad}'
tar -xJf "$work/helper-parent/uke-battery-observer-1.4.1.tar.xz" -C "$work/helper"
old=$work/helper/uke-battery-observer-1.4.1
(cd "$old";sha256sum -c --strict --quiet SOURCE-SHA256SUMS)
# The signed native91 receipt pins the exact charge code; do not rerun a
# separate source-check build or replace the complete current Control tree.
proof=$(jq -er .native91.build "$manifest")
url=$(curl -qfsSL --max-time 40 "https://copr.fedorainfracloud.org/api_3/build-chroot/$proof/fedora-rawhide-aarch64" | jq -er 'if .state=="succeeded" then .result_url else error("Native91 unqualified") end')
curl -qfsSL --max-time 60 "${url}uke-kernel-migration-check-7.3.3-91.fc46.aarch64.rpm" -o "$work/proof.rpm"
printf '%s  %s\n' "$(jq -er .native91.receipt_sha256 "$manifest")" "$work/proof.rpm" | sha256sum -c --strict --quiet -
rpm2cpio "$work/proof.rpm" | cpio -i --to-stdout --quiet '*control-charge-source-qualification.json' > "$work/proof.json"
jq -e '.source_commit=="89373a27cab679da28a39179e58fd34099f857a6" and .parent_control_build==11300991 and .actual_helper_parent_build==11250670 and .new_runtime_packages==0 and .new_services==0' "$work/proof.json" >/dev/null
(cd "$workspace";jq -r '.source_assets[]|.sha256+"  "+.path' "$work/proof.json" | sha256sum -c --strict --quiet -)
while IFS= read -r path;do
 install -Dm644 "$old/$path" "$work/overlay/runtime/$path"
done < <(jq -r '.runtime_payload_paths[]' "$manifest")
while IFS= read -r path;do
 install -m644 "$workspace/uke-platform-runtime/$path" "$work/overlay/runtime/$path"
done < <(jq -r '.native91_runtime_replacements[]' "$manifest")
(cd "$work/overlay/runtime";jq -r '.unchanged_runtime_data[]|.sha256+"  "+.path' "$manifest" | sha256sum -c --strict --quiet -)
for path in apply-charge-overlay.sh charge-bridge-native91.patch charge-unit-state.sh charge-fold-source.json;do
 install -Dm644 "$owner/packaging/$path" "$work/overlay/packaging/$path"
done
install -Dm644 "$owner/tests/charge-unit-state.sh" "$work/overlay/tests/charge-unit-state.sh"
install -m644 "$owner/docs/CHARGE-CONSOLIDATION.md" "$work/overlay/CHARGE-CONSOLIDATION.md"
(cd "$work/overlay";find . -type f ! -name SHA256SUMS -print0 | sort -z | xargs -0 sha256sum > SHA256SUMS)
epoch=$(git -C "$workspace" show -s --format=%ct HEAD)
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf "$output/uke-control-charge.tar.xz" -C "$work/overlay" .
install -m644 "$owner/packaging/fold-charge-spec.sh" "$owner/packaging/copr-control-charge-fold.sh" "$manifest" "$output/"
bash "$owner/packaging/fold-charge-spec.sh" "$output/uke-control.spec" "$owner/packaging/charge-unit-state.sh"
[[ $(grep -Fc 'application_version="2.16.0"' "$output/control-version.hpp") == 1 ]]
sed -i 's/application_version="2.16.0"/application_version="2.17.0"/' "$output/control-version.hpp"
[[ $(rpmspec -q --srpm --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$output/uke-control.spec") == uke-control-2.17.0-1.fc46 ]]
rpmspec -q --qf '%{NAME}\n' "$output/uke-control.spec" | sort > "$work/names"
printf '%s\n' uke-control uke-control-client-devel uke-control-tui | cmp - "$work/names"
rpmspec -q --requires "$output/uke-control.spec" > "$work/requires"
awk '/^uke-control(-tui|-client)?(\([^)]*\))? = /{if($NF!="2.17.0-1.fc46")bad=1;n++}END{exit bad||n<5}' "$work/requires"
rpmspec -q --provides "$output/uke-control.spec" > "$work/provides"
grep -Fx 'uke-charge-control = 1.4.1-8.fc46' "$work/provides"
grep -Fx 'uke-charge-control(aarch-64) = 1.4.1-8.fc46' "$work/provides"
bash "$owner/tests/charge-unit-state.sh" "$output/uke-control.spec"
printf 'Control217 charge source: actual216r2 parent and native91 delta,retained helper unit/profile payload,three existing cohort names; RPM/device/electrical parity pending.\n'
