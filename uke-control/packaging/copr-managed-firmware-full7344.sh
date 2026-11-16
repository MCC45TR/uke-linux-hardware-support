#!/usr/bin/bash
# COPR only; never publish a current-source admission before full success.
set -Eeuo pipefail
[[ $# == 1 && ${UKE_COPR_SOURCE_WORKER:-} == 1 && $(uname -m) == aarch64 && ! -e /etc/senemos/kernel-update.conf ]]
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
result=$(realpath -m "$1")
[[ ! -e $result && ! -L $result ]]
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
curl -qfsSL --max-time 45 https://copr.fedorainfracloud.org/api_3/build/11333980 > "$work/full.json"
jq -e '.state=="succeeded" and .source_package.name=="senemos-uke-linux-kernel-mainline-unstable" and .source_package.version=="7.3.4-4"' "$work/full.json" >/dev/null
curl -qfsSL --max-time 45 https://copr.fedorainfracloud.org/api_3/build/source-build-config/11333980 > "$work/worker.json"
grep -Fxq 'commit=4a424e96c528920d59541fa9cc2aa426a90f296e' < <(jq -j .source_dict.script "$work/worker.json")
mkdir "$result" "$work/parent"
curl -qfsSL --max-time 120 --max-filesize 1048576 https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/srpm-builds/11333377/uke-cdsp-pogo-firmware-0.2-13.src.rpm -o "$work/parent.src.rpm"
printf '%s  %s\n' 0aae8ce2bdb179f8e017f37fbad8799fb49f038e43a0d46c65415ea68d31db67 "$work/parent.src.rpm" | sha256sum -c --strict --quiet -
rpm2cpio "$work/parent.src.rpm" | cpio -t --quiet > "$work/paths"
awk '{name=$0;sub(/^\.\//,"",name);if(name!~/^[A-Za-z0-9_.+-]+$/||seen[name]++)bad=1;count++}END{if(bad||count!=9)exit 1}' "$work/paths"
rpm2cpio "$work/parent.src.rpm" | (cd "$work/parent";cpio -idm --quiet --no-absolute-filenames --no-preserve-owner)
(cd "$work/parent";find . -maxdepth 1 -type f ! -name uke-cdsp-pogo-firmware.spec -print0 | sort -z | xargs -0 sha256sum) > "$work/inputs.sha256"
bash "$owner/packaging/fold-managed-firmware-full7344.sh" "$work/parent/uke-cdsp-pogo-firmware.spec" "$work/new.spec"
cp -a "$work/parent/." "$result/"
install -m644 "$work/new.spec" "$result/uke-cdsp-pogo-firmware.spec"
install -m644 "$owner/packaging/managed-firmware-full7344.patch" "$result/"
(cd "$result";sha256sum -c --strict --quiet "$work/inputs.sha256")
tar -xzf "$result/uke-cdsp-pogo-firmware-0.2.tar.gz" -C "$work"
tree=$work/uke-cdsp-pogo-firmware-0.2
install -m644 "$result/unstable-release.hpp" "$tree/uke-cdsp-pogo-firmware/src/"
for patch in managed-firmware-version managed-firmware-idle28 managed-firmware-factory-mcu managed-firmware-full734 managed-firmware-full7343;do
 (cd "$tree";patch --batch --fuzz=0 -p1 < "$result/$patch.patch")
done
install -m644 "$result/factory-mcu-test.hpp" "$tree/uke-cdsp-pogo-firmware/tests/"
printf '%s  %s\n' d817526d696999ec7669fd0f756cbff9433b4b331ba22217f0e81a02989d6a17 "$tree/uke-cdsp-pogo-firmware/src/managed.hpp" | sha256sum -c --strict --quiet -
printf '%s  %s\n' fe7608d46d4d8abd27a04f0098be52b0c1a680647b2386871e52085aac566037 "$tree/uke-cdsp-pogo-firmware/tests/managed.cpp" | sha256sum -c --strict --quiet -
cp "$tree/uke-cdsp-pogo-firmware/src/managed.hpp" "$work/before.hpp"
cp "$tree/uke-cdsp-pogo-firmware/tests/managed.cpp" "$work/before-managed.cpp"
(cd "$tree";patch --batch --fuzz=0 -p1 < "$result/managed-firmware-full7344.patch";patch --batch --fuzz=0 -R -p1 < "$result/managed-firmware-full7344.patch")
cmp "$work/before.hpp" "$tree/uke-cdsp-pogo-firmware/src/managed.hpp"
cmp "$work/before-managed.cpp" "$tree/uke-cdsp-pogo-firmware/tests/managed.cpp"
[[ $(rpmspec --target aarch64 --define 'dist .fc46' -q --srpm --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$result/uke-cdsp-pogo-firmware.spec") == uke-cdsp-pogo-firmware-0.2-14.fc46 ]]
printf 'Managed14 source assembled with all managed13 inputs and exact successful7344 admission; native checks remain required.\n'
