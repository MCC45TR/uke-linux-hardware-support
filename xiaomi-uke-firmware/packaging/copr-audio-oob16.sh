#!/usr/bin/bash
# COPR-only data RPM assembly; no compiler or device access.
set -Eeuo pipefail
[[ $# == 1 && ${UKE_COPR_SOURCE_WORKER:-} == 1 && $(uname -m) == aarch64 && ! -e /etc/senemos/kernel-update.conf ]]
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
result=$(realpath -m "$1")
[[ ! -e $result && ! -L $result ]]
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
mkdir "$result" "$work/parent"
curl -qfsSL --max-time 240 --max-filesize 33554432 https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/srpm-builds/11309337/uke-firmware-3.0.304.0-15.src.rpm -o "$work/parent.src.rpm"
printf '%s  %s\n' 1103e7bcf7fa5f8fe60ebecfca7be0a9b9b49170bdec4c5c2cbdc42cfa86b45a "$work/parent.src.rpm" | sha256sum -c --strict --quiet -
rpm2cpio "$work/parent.src.rpm" | cpio -t --quiet > "$work/paths"
awk '{name=$0;sub(/^\.\//,"",name);if(name!~/^[A-Za-z0-9_.+-]+$/||seen[name]++)bad=1;count++}END{if(bad||count!=3)exit 1}' "$work/paths"
rpm2cpio "$work/parent.src.rpm" | (cd "$work/parent";cpio -idm --quiet --no-absolute-filenames --no-preserve-owner)
(cd "$work/parent";sha256sum *.tar.xz) > "$work/inputs.sha256"
bash "$owner/packaging/fold-audio-oob16.sh" "$work/parent/uke-firmware.spec" "$result/uke-firmware.spec"
cp "$work/parent/"*.tar.xz "$result/"
install -m644 "$owner/src/uke-speaker-data.conf" "$owner/tests/audio-oob-cache.sh" "$owner/manifests/audio-oob-20261116.json" "$result/"
curl -qfsSL --max-time 120 --max-filesize 2838166 https://raw.githubusercontent.com/Xiaomi-Pad-7-Pro-Resources/android_vendor_xiaomi_uke/87f59fc5c2ef1ee04e494e5258853d36a3346059/proprietary/odm/etc/acdbdata/Forte/Forte_acdb_cal.acdb -o "$result/Forte_acdb_cal.acdb"
[[ $(stat -c %s "$result/Forte_acdb_cal.acdb") == 2838166 ]]
printf '%s  %s\n' bd3f9b8834b80c23f86241756e263e36ddf92b2b8993ed83ff71c310af952f3e "$result/Forte_acdb_cal.acdb" | sha256sum -c --strict --quiet -
(cd "$result";sha256sum -c --strict --quiet "$work/inputs.sha256")
[[ $(rpmspec --target aarch64 --define 'dist .fc46' -q --srpm --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$result/uke-firmware.spec") == uke-firmware-3.0.304.0-16.fc46 ]]
printf 'Firmware16 spec and all pinned inputs prepared; original15 payloads/notices and recipe checks retained, no private unit data or runtime daemon.\n'
