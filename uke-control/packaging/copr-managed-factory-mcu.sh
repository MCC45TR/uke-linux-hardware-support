#!/usr/bin/bash
# COPR source assembly only; native production and sanitizer compilation in RPM.
set -Eeuo pipefail
[[ $# == 1 && ${UKE_COPR_SOURCE_WORKER:-} == 1 && $(uname -m) == aarch64 && ! -e /etc/senemos/kernel-update.conf ]]
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
result=$(realpath -m "$1")
[[ ! -e $result && ! -L $result ]]
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
mkdir "$result" "$work/parent"
curl -qfsSL --max-time 120 --max-filesize 1048576 https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/srpm-builds/11331717/uke-cdsp-pogo-firmware-0.2-10.src.rpm -o "$work/parent.src.rpm"
printf '%s  %s\n' 37abf0e2ca2608238073c1c3544df63c2556e0bb502b029b10c4e2e1f48dfbc4 "$work/parent.src.rpm" | sha256sum -c --strict --quiet -
rpm2cpio "$work/parent.src.rpm" | cpio -t --quiet > "$work/paths"
awk '{name=$0;sub(/^\.\//,"",name);if(name!~/^[A-Za-z0-9_.+-]+$/||seen[name]++)bad=1;count++}END{if(bad||count!=5)exit 1}' "$work/paths"
rpm2cpio "$work/parent.src.rpm" | (cd "$work/parent";cpio -idm --quiet --no-absolute-filenames --no-preserve-owner)
(cd "$work/parent";find . -maxdepth 1 -type f ! -name uke-cdsp-pogo-firmware.spec -print0 | sort -z | xargs -0 sha256sum) > "$work/inputs.sha256"
bash "$owner/packaging/fold-managed-factory-mcu.sh" "$work/parent/uke-cdsp-pogo-firmware.spec" "$work/new.spec"
cp -a "$work/parent/." "$result/"
install -m644 "$work/new.spec" "$result/uke-cdsp-pogo-firmware.spec"
install -m644 "$owner/packaging/managed-firmware-factory-mcu.patch" "$owner/tests/factory-mcu-test.hpp" "$result/"
printf '%s  %s\n' 7cdb6ba8927bf0335236e18328cc37da67c84eb637520bbbfd2db19cfb6d7e3a "$result/managed-firmware-factory-mcu.patch" \
 0a1dbb8faa4c3244b31abe16ed125c69769c967e3eb6ff08157755d57b962794 "$result/factory-mcu-test.hpp" | sha256sum -c --strict --quiet -
(cd "$result";sha256sum -c --strict --quiet "$work/inputs.sha256")
tar -xzf "$result/uke-cdsp-pogo-firmware-0.2.tar.gz" -C "$work"
tree=$work/uke-cdsp-pogo-firmware-0.2
install -m644 "$result/unstable-release.hpp" "$tree/uke-cdsp-pogo-firmware/src/"
for patch in managed-firmware-version managed-firmware-idle28 managed-firmware-factory-mcu;do
 (cd "$tree";patch --batch --fuzz=0 -p1 < "$result/$patch.patch")
done
install -m644 "$result/factory-mcu-test.hpp" "$tree/uke-cdsp-pogo-firmware/tests/"
[[ $(rpmspec --target aarch64 --define 'dist .fc46' -q --srpm --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$result/uke-cdsp-pogo-firmware.spec") == uke-cdsp-pogo-firmware-0.2-11.fc46 ]]
printf 'Managed firmware11 complete10 sources/recipe retained; protected factory file precedes unchanged read-only stock MCU parsing; native fixtures remain in RPM build.\n'
