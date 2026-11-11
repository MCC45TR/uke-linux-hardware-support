#!/usr/bin/bash
# COPR source worker only: one existing runtime, with the mandatory TUI retained.
set -Eeuo pipefail
test ! -e /etc/senemos/kernel-update.conf
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
output=$(realpath -m "${1:?COPR result directory}")
[[ ! -e $output ]]
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
curl -qfsSL --retry 2 --max-time 180 \
 https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/srpm-builds/11291213/uke-control-2.13.0-1.src.rpm -o "$work/parent.rpm"
printf '%s  %s\n' 432db4bc4e40eda63700af4ed0b9d22ad6555a3a7c35d566f107dc3abae23c43 "$work/parent.rpm" | sha256sum -c --strict --quiet -
rpm2cpio "$work/parent.rpm" > "$work/parent.cpio"
cpio -t --quiet < "$work/parent.cpio" | awk '/^\/|(^|\/)\.\.($|\/)/{bad=1}END{exit bad}'
mkdir "$output" "$work/overlay" "$work/runtime"
(cd "$output";cpio -idm --quiet --no-absolute-filenames < "$work/parent.cpio")
curl -qfsSL --retry 2 --max-time 180 \
 https://github.com/MCC45TR/uke-linux-hardware-support/releases/download/copr-consolidation-archive-20261101/uke-speaker-runtime-0.4-3.src.rpm -o "$work/runtime.rpm"
printf '%s  %s\n' 22f97cc686e6746cb035865fbf9ba6e7161d3ba00034f8dc7742dd942db286c4 "$work/runtime.rpm" | sha256sum -c --strict --quiet -
rpm2cpio "$work/runtime.rpm" > "$work/runtime.cpio"
cpio -t --quiet < "$work/runtime.cpio" | awk '/^\/|(^|\/)\.\.($|\/)/{bad=1}END{exit bad}'
(cd "$work/runtime";cpio -idm --quiet --no-absolute-filenames < "$work/runtime.cpio")
install -m644 "$work/runtime/uke-speaker-runtime-0.4.tar.gz" "$output/retained-speaker-runtime-0.4-3.tar.gz"
for file in packaging/speaker-runtime.patch packaging/speaker-state.sh packaging/SPEAKER-RUNTIME-GPL-2.0 \
 tests/speaker-ensure-loaded.sh tests/speaker-provider.sh tests/speaker-state.sh docs/SPEAKER-RUNTIME-CONSOLIDATION.md;do
 install -Dm644 "$owner/$file" "$work/overlay/$file"
done
tar --sort=name --mtime=@1794232800 --owner=0 --group=0 --numeric-owner -cJf "$output/uke-control-speaker-runtime.tar.xz" -C "$work/overlay" .
install -m644 "$owner/packaging/fold-speaker-spec.sh" "$owner/packaging/copr-control-speaker.sh" "$output/"
bash "$owner/packaging/fold-speaker-spec.sh" "$output/uke-control.spec"
sed -i 's/application_version="2.13.0"/application_version="2.14.0"/' "$output/control-version.hpp"
[[ $(rpmspec -q --srpm --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$output/uke-control.spec") == uke-control-2.14.0-1.fc46 ]]
rpmspec -q --qf '%{NAME}\n' "$output/uke-control.spec" | sort > "$work/names"
printf '%s\n' uke-control uke-control-client-devel uke-control-tui > "$work/expected"
cmp "$work/names" "$work/expected"
bash "$owner/tests/kernel-consolidation.sh" "$output/uke-control.spec"
bash "$owner/tests/speaker-state.sh" "$output/uke-control.spec"
printf 'Control speaker source prepared from immutable213/runtime043; three existing output names, mandatory matching TUI, bounded migration and no new service.\n'
