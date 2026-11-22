#!/usr/bin/bash
# COPR source worker only. Add a session layer to the current complete cohort.
set -Eeuo pipefail
test ! -e /etc/senemos/kernel-update.conf
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
output=$(realpath -m "${1:?COPR source result}")
[[ ! -e $output ]]
stage=$(mktemp -d)
trap 'find "$stage" -depth -delete' EXIT
curl -qfsSL --retry 2 --max-time 180 \
 https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/srpm-builds/11283518/uke-control-2.9.0-3.src.rpm -o "$stage/parent.rpm"
printf '%s  %s\n' fbaef727ceaed7b0be7f8a37c6c0866b88d9d921f37321a904004a8b3aa6cf71 "$stage/parent.rpm" | sha256sum -c --strict --quiet -
rpm2cpio "$stage/parent.rpm" > "$stage/parent.cpio"
cpio -t --quiet < "$stage/parent.cpio" | awk '/^\/|(^|\/)\.\.($|\/)/{bad=1}END{exit bad}'
mkdir "$output" "$stage/session"
(cd "$output";cpio -idm --quiet --no-absolute-filenames < "$stage/parent.cpio")
cp "$output/uke-control.spec" "$stage/parent.spec"
cp "$output/control-version.hpp" "$stage/parent-version.hpp"
for file in src/session/speaker-orientation.cpp tests/session/speaker-orientation.cpp tests/session/speaker-orientation.sh packaging/uke-speaker-orientation.desktop docs/SPEAKER-ORIENTATION.md;do
 install -Dm644 "$owner/$file" "$stage/session/$file"
done
install -m644 "$owner/../LICENSE" "$stage/session/LICENSE"
tar --sort=name --mtime=@1794139200 --owner=0 --group=0 --numeric-owner -cJf "$output/uke-control-session-bridge.tar.xz" -C "$stage/session" .
install -m644 "$owner/packaging/copr-control-session.sh" "$owner/packaging/fold-session-spec.sh" "$output/"
bash "$owner/packaging/fold-session-spec.sh" "$output/uke-control.spec"
sed 's/application_version="2.9.0"/application_version="2.10.0"/' "$stage/parent-version.hpp" > "$output/control-version.hpp"
[[ $(rpmspec -q --srpm --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$output/uke-control.spec") == uke-control-2.10.0-1.fc46 ]]
rpmspec -q --qf '%{NAME}\n' "$output/uke-control.spec" | sort > "$stage/names"
printf '%s\n' uke-control uke-control-client-devel uke-control-tui > "$stage/expected"
cmp "$stage/names" "$stage/expected"
rpmspec -P "$output/uke-control.spec" > "$stage/expanded"
grep -Fx 'Requires: uke-control-tui(aarch-64) = 2.10.0-1.fc46' "$stage/expanded"
grep -Fx 'Provides: uke-speaker-orientation = 0.2.0-3.fc46' "$stage/expanded"
grep -Fx 'Obsoletes: uke-speaker-orientation <= 0.2.0-2.fc46' "$stage/expanded"
bash "$owner/tests/kernel-consolidation.sh" "$output/uke-control.spec"
printf 'Control session source prepared from admitted293; original source archives/lifecycle retained, one main runtime plus existing TUI/SDK only.\n'
