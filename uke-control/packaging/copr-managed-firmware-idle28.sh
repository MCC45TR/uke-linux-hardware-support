#!/usr/bin/bash
# COPR source assembly only; retain the complete published firmware9 source.
set -Eeuo pipefail
[[ $# == 1 && ${UKE_COPR_SOURCE_WORKER:-} == 1 && $(uname -m) == aarch64 && ! -e /etc/senemos/kernel-update.conf ]]
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
result=$(realpath -m "$1")
[[ ! -e $result && ! -L $result ]]
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
mkdir "$result" "$work/parent"
curl -qfsSL --max-time 120 --max-filesize 1048576 https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/srpm-builds/11283443/uke-cdsp-pogo-firmware-0.2-9.src.rpm -o "$work/parent.src.rpm"
printf '%s  %s\n' 9fba3e56e86a8799f030de3b00115fed78a0cc2ac278d9486d950532422e7ae7 "$work/parent.src.rpm" | sha256sum -c --strict --quiet -
rpm2cpio "$work/parent.src.rpm" | cpio -t --quiet > "$work/paths"
awk '{name=$0;sub(/^\.\//,"",name);if(name!~/^[A-Za-z0-9_.+-]+$/||seen[name]++)bad=1;count++}END{if(bad||count!=4)exit 1}' "$work/paths"
rpm2cpio "$work/parent.src.rpm" | (cd "$work/parent";cpio -idm --quiet --no-absolute-filenames --no-preserve-owner)
parent=$work/parent/uke-cdsp-pogo-firmware.spec
printf '%s  %s\n' 0a9a2334a986454410915dde9af652c1a8984524968500a57ff40639bcb08d81 "$parent" | sha256sum -c --strict --quiet -
(cd "$work/parent";sha256sum *.tar.gz unstable-release.hpp managed-firmware-version.patch) > "$work/inputs.sha256"
bash "$owner/packaging/fold-managed-firmware-idle28.sh" "$parent" "$result/uke-cdsp-pogo-firmware.spec"
cp "$work/parent/"{uke-cdsp-pogo-firmware-0.2.tar.gz,unstable-release.hpp,managed-firmware-version.patch} "$result/"
install -m644 "$owner/packaging/managed-firmware-idle28.patch" "$result/"
printf '%s  %s\n' 8ffe830bbfb670747c1b92502d9ce2d4b4e24cbe2c642c8359fe4b9cde14bb89 "$result/managed-firmware-idle28.patch" | sha256sum -c --strict --quiet -
(cd "$result";sha256sum -c --strict --quiet "$work/inputs.sha256")
tar -xzf "$result/uke-cdsp-pogo-firmware-0.2.tar.gz" -C "$work"
tree=$work/uke-cdsp-pogo-firmware-0.2
install -m644 "$result/unstable-release.hpp" "$tree/uke-cdsp-pogo-firmware/src/"
(cd "$tree";patch --batch --fuzz=0 -p1 < "$result/managed-firmware-version.patch")
printf '%s  %s\n' 1e0762d9f1f4fc647f3dbdee6e168ffaf8eef5654a9834788c09602bbf484bd4 "$tree/uke-cdsp-pogo-firmware/src/managed.hpp" | sha256sum -c --strict --quiet -
(cd "$tree";patch --batch --fuzz=0 -p1 < "$result/managed-firmware-idle28.patch")
[[ $(rpmspec --target aarch64 --define 'dist .fc46' -q --srpm --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$result/uke-cdsp-pogo-firmware.spec") == uke-cdsp-pogo-firmware-0.2-10.fc46 ]]
printf 'Firmware10: exact full28 tuple and additional refusal fixtures; full9 recipe inverse and all original source bytes retained. Native checks run in COPR RPM build.\n'
