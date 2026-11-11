#!/usr/bin/bash
# Native COPR build only. No new runtime package, daemon or sensor consumer.
set -Eeuo pipefail
test ! -e /etc/senemos/kernel-update.conf
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
output=$(realpath -m "${1:?COPR result directory}")
[[ ! -e $output ]]
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
curl -qfsSL --retry 2 --max-time 180 \
 https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/srpm-builds/11290779/uke-control-2.12.3-1.src.rpm -o "$work/parent.rpm"
printf '%s  %s\n' b9a1920df09fffbfaef328e33df1470ffc62c1605719147f09f7867b757de3a3 "$work/parent.rpm" | sha256sum -c --strict --quiet -
rpm2cpio "$work/parent.rpm" > "$work/parent.cpio"
cpio -t --quiet < "$work/parent.cpio" | awk '/^\/|(^|\/)\.\.($|\/)/{bad=1}END{exit bad}'
mkdir "$output" "$work/overlay"
(cd "$output";cpio -idm --quiet --no-absolute-filenames < "$work/parent.cpio")
for file in src/brightness-curve.cpp src/brightness-curve-config.hpp src/oss-brightness-curve.hpp \
 tests/brightness-curve.cpp tests/brightness-curve.sh packaging/brightness-curve-cli.patch \
 packaging/30-uke-brightness-curve.sh docs/BRIGHTNESS-CURVE.md;do
 install -Dm644 "$owner/$file" "$work/overlay/$file"
done
tar --sort=name --mtime=@1794225600 --owner=0 --group=0 --numeric-owner -cJf "$output/uke-control-brightness-curve.tar.xz" -C "$work/overlay" .
install -m644 "$owner/packaging/fold-brightness-spec.sh" "$owner/packaging/copr-control-brightness.sh" "$output/"
bash "$owner/packaging/fold-brightness-spec.sh" "$output/uke-control.spec"
sed -i 's/application_version="2.12.3"/application_version="2.13.0"/' "$output/control-version.hpp"
[[ $(rpmspec -q --srpm --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$output/uke-control.spec") == uke-control-2.13.0-1.fc46 ]]
rpmspec -q --qf '%{NAME}\n' "$output/uke-control.spec" | sort > "$work/names"
printf '%s\n' uke-control uke-control-client-devel uke-control-tui > "$work/expected"
cmp "$work/names" "$work/expected"
printf 'Control brightness source prepared from admitted2.12.3; same three output names, exact retained updater/TUI/services and no sensor daemon.\n'
