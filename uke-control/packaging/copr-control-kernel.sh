#!/usr/bin/env bash
# COPR source-worker only. Retain both current producers without binary repacking.
set -Eeuo pipefail
test ! -e /etc/senemos/kernel-update.conf
[[ $(rpm --eval '%{_host_cpu}') == aarch64 ]]
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
output=$(realpath -m "${1:?Source result directory}")
[[ ! -e $output || (-d $output && -z $(find "$output" -mindepth 1 -maxdepth 1 -print -quit)) ]]
stage=$(mktemp -d);trap 'find "$stage" -depth -delete' EXIT
mkdir -p "$stage/control" "$stage/boot" "$output"
for entry in \
 'control 11268045 uke-control-2.8.1-26.src.rpm cfaf74229aaebcae300e85a468ba04ab0e54aba8bc50a237707418771f743391' \
 'boot 11272089 uke-boot-integration-1.8.102-1.fc46.src.rpm 06208c79c6271e8c8542d0693b47d9285007cbfd184e2504e2d48b79abb72a1f';do
 read -r kind id name hash <<< "$entry"
 curl -q -fLsS --retry 2 --max-time 180 --proto '=https' --proto-redir '=https' \
  "https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/srpm-builds/$id/$name" -o "$stage/$kind.src.rpm"
 printf '%s  %s\n' "$hash" "$stage/$kind.src.rpm" | sha256sum -c --strict -
 rpm2cpio "$stage/$kind.src.rpm" > "$stage/input.cpio"
 cpio -t --quiet < "$stage/input.cpio" | awk '/^\/|(^|\/)\.\.($|\/)/ {bad=1} END {exit bad}'
 (cd "$stage/$kind";cpio -idm --quiet --no-absolute-filenames < "$stage/input.cpio")
done
find "$stage/control" -maxdepth 1 -type f ! -name '*.spec' -exec cp -t "$output" -- {} +
cp "$stage/boot/uke-boot-integration-1.8.102.tar.xz" "$output/"
install -m644 "$owner/packaging/60-uke-filelists.conf" "$output/"
install -m644 "$owner/packaging/merge-kernel-spec.cpp" "$owner/packaging/copr-control-kernel.sh" "$owner/docs/KERNEL-CONSOLIDATION.md" "$output/"
install -m644 "$owner/tests/kernel-consolidation.sh" "$output/"
printf '%s\n' '// SPDX-License-Identifier: MIT' '#pragma once' 'namespace uke {' \
 'inline constexpr const char* application_version="2.9.0";' '}' > "$output/control-version.hpp"
g++ -std=c++20 -O2 -Wall -Wextra -Werror "$owner/packaging/merge-kernel-spec.cpp" -o "$stage/merge-kernel-spec"
"$stage/merge-kernel-spec" "$stage/control/uke-control.spec" "$stage/boot/uke-boot-integration.spec" > "$output/uke-control.spec"
cp "$output/uke-control.spec" "$output/control-consolidated-spec.txt"
[[ $(rpmspec -q --srpm --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$output/uke-control.spec") == uke-control-2.9.0-1.fc46 ]]
rpmspec -q --qf '%{NAME}\n' "$output/uke-control.spec" | sort > "$stage/binaries"
printf '%s\n' uke-control uke-control-client-devel > "$stage/expected-binaries"
cmp "$stage/binaries" "$stage/expected-binaries"
rpmspec -P "$output/uke-control.spec" > "$stage/expanded.spec"
bash "$owner/tests/kernel-consolidation.sh" "$output/uke-control.spec"
# All existing capabilities survive the source-family move.
for capability in \
 'uke-kernel-update(alternating-checkpoint-v2)' 'uke-kernel-update(seamless-policy-v2)' \
 'uke-kernel-update(rollback-policy-v1)' 'uke-kernel-update(final-transaction-v2)' \
 'uke-kernel-update(qualified-firmware-cohort-v1)' 'uke-installation-profile(rtc-secure-offset-v1)' \
 'uke-control(update-status-checkpoint-v2)' 'uke-control(frontend-v1)';do
 grep -Fq "Provides: $capability = " "$stage/expanded.spec"
done
[[ $(grep -Ec '^%transfiletriggerin .*-- /usr/lib/senemos$' "$stage/expanded.spec") == 1 ]]
grep -Fq '/usr/bin/uke-kernel-update auto' "$stage/expanded.spec"
grep -Fq 'optional_metadata_types=filelists' "$output/60-uke-filelists.conf"
grep -Fx 'Requires: systemd-udev' "$stage/expanded.spec"
! grep -Fx 'Requires: /usr/lib/systemd/systemd-growfs' "$stage/expanded.spec"
[[ $(grep -Fc '%config(noreplace) /etc/senemos/rollback.conf' "$stage/expanded.spec") == 1 ]]
[[ $(grep -Fc '%config(noreplace) /etc/senemos/kernel-update.conf' "$stage/expanded.spec") == 1 ]]
[[ $(grep -Fc '%config(noreplace) /etc/senemos/userdata-update.conf' "$stage/expanded.spec") == 1 ]]
# Every retained archive remains byte-identical; only the new source recipe and
# explicit version/configuration inputs are added.
for file in "$stage/control/"*;do
 [[ $file != *.spec ]] || continue
 cmp "$file" "$output/$(basename "$file")"
done
cmp "$stage/boot/uke-boot-integration-1.8.102.tar.xz" "$output/uke-boot-integration-1.8.102.tar.xz"
printf 'Control26 and updater102 source inputs retained; one runtime plus existing SDK, preserved A/B/rollback/RTC APIs and one native DNF trigger generated.\n'
