#!/usr/bin/bash
# COPR-only data RPM source assembly; no compiler or device access.
set -Eeuo pipefail
[[ $# == 1 && ${UKE_COPR_SOURCE_WORKER:-} == 1 && $(uname -m) == aarch64 && ! -e /etc/senemos/kernel-update.conf ]]
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
result=$(realpath -m "$1")
[[ ! -e $result && ! -L $result ]]
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
mkdir "$result" "$work/parent"
curl -qfsSL --max-time 240 --max-filesize 33554432 https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/srpm-builds/11331847/uke-firmware-3.0.304.0-16.src.rpm -o "$work/parent.src.rpm"
printf '%s  %s\n' ae0d324047da78fd359a9c7439417fd96c62584bf055eadae5c59c6970ea1d04 "$work/parent.src.rpm" | sha256sum -c --strict --quiet -
rpm2cpio "$work/parent.src.rpm" | cpio -t --quiet > "$work/paths"
awk '{name=$0;sub(/^\.\//,"",name);if(name!~/^[A-Za-z0-9_.+-]+$/||seen[name]++)bad=1;count++}END{if(bad||count!=7)exit 1}' "$work/paths"
rpm2cpio "$work/parent.src.rpm" | (cd "$work/parent";cpio -idm --quiet --no-absolute-filenames --no-preserve-owner)
(cd "$work/parent";find . -maxdepth 1 -type f ! -name uke-firmware.spec -print0 | sort -z | xargs -0 sha256sum) > "$work/inputs.sha256"
bash "$owner/packaging/fold-pogo-mcu17.sh" "$work/parent/uke-firmware.spec" "$work/new.spec"
cp -a "$work/parent/." "$result/"
install -m644 "$work/new.spec" "$result/uke-firmware.spec"
install -m644 "$owner/manifests/pogo-mcu-20261116.json" "$result/"
curl -qfsSL --max-time 90 --max-filesize 82740 https://raw.githubusercontent.com/Xiaomi-Pad-7-Pro-Resources/android_vendor_xiaomi_sm8635-common/d4bf71b32fc8e82fbe6ee2dcbbdd4272f1036154/proprietary/odm/firmware/MCU_Upgrade.bin -o "$result/MCU_Upgrade.bin"
[[ $(stat -c %s "$result/MCU_Upgrade.bin") == 82740 ]]
printf '%s  %s\n' b17fa938cf5613dfa550179d6fc40cfbaa831d534a16a22282e2904e845fa7c4 "$result/MCU_Upgrade.bin" | sha256sum -c --strict --quiet -
(cd "$result";sha256sum -c --strict --quiet "$work/inputs.sha256")
[[ $(rpmspec --target aarch64 --define 'dist .fc46' -q --srpm --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$result/uke-firmware.spec") == uke-firmware-3.0.304.0-17.fc46 ]]
printf 'Firmware17 pinned public MCU matches existing Uke whole-file admission; complete16 inputs/recipe/notices retained, no controller start.\n'
