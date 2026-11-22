#!/usr/bin/bash
# COPR only: fold the existing native client into the self-contained Control.
set -Eeuo pipefail
test ! -e /etc/senemos/kernel-update.conf
[[ $# == 1 ]]
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
workspace=$(dirname "$owner")
output=$(realpath -m "$1")
[[ ! -e $output ]]
bash "$owner/packaging/copr-control-session-release.sh" "$output"
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
mkdir "$work/nfc"
for path in 70-uke-nfc-ir.rules README.md uapi-source.json;do install -m644 "$workspace/uke-nfc-ir/$path" "$work/nfc/$path";done
for dir in src include tests LICENSES;do cp -a "$workspace/uke-nfc-ir/$dir" "$work/nfc/$dir";done
install -m644 "$workspace/LICENSE" "$work/nfc/LICENSE"
install -m644 "$owner/docs/NFC-CONSOLIDATION.md" "$work/NFC-CONSOLIDATION.md"
(cd "$workspace";jq -r ' .source_assets[]|.sha256+"  "+.path' "$owner/packaging/nfc-source.json" | sha256sum -c --strict --quiet -)
install -m644 "$owner/packaging/nfc-source.json" "$work/NFC-SOURCE.json"
tar --sort=name --mtime=@1794328200 --owner=0 --group=0 --numeric-owner -cJf "$output/uke-control-nfc.tar.xz" -C "$work" nfc NFC-CONSOLIDATION.md NFC-SOURCE.json
install -m644 "$owner/packaging/fold-nfc-spec.sh" "$owner/packaging/copr-control-nfc.sh" "$output/"
bash "$owner/packaging/fold-nfc-spec.sh" "$output/uke-control.spec"
[[ $(grep -Fc 'application_version="2.15.0"' "$output/control-version.hpp") == 1 ]]
sed -i 's/application_version="2.15.0"/application_version="2.16.0"/' "$output/control-version.hpp"
[[ $(rpmspec -q --srpm --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$output/uke-control.spec") == uke-control-2.16.0-1.fc46 ]]
rpmspec -q --qf '%{NAME}\n' "$output/uke-control.spec" | sort > "$work/names"
printf '%s\n' uke-control uke-control-client-devel uke-control-tui > "$work/expected"
cmp "$work/names" "$work/expected"
rpmspec -q --requires "$output/uke-control.spec" > "$work/requires"
rpmspec -q --provides "$output/uke-control.spec" > "$work/provides"
awk '/^uke-control(-tui|-client)?(\([^)]*\))? = / {if($NF!="2.16.0-1.fc46")bad=1;n++} END{exit bad||n<5}' "$work/requires"
for cap in 'uke-control(aarch-64)' 'uke-control-tui(aarch-64)' 'uke-control-client(aarch-64)';do grep -Fx "$cap = 2.16.0-1.fc46" "$work/provides";done
grep -Fx 'uke-nfc-ir = 1.1.0-3.fc46' "$work/provides"
grep -Fx 'uke-device-access(nfc-ir-v1) = 1' "$work/provides"
grep -Fx 'uke-nfc-tag-client(abi1) = 1' "$work/provides"
bash "$owner/tests/kernel-consolidation.sh" "$output/uke-control.spec"
bash "$owner/tests/speaker-state.sh" "$output/uke-control.spec"
printf 'Control216 NFC source: unchanged qualified tag client/UAPI/session ACLs,all215 features,three existing output names and mandatory matching TUI; no daemon/runtime family added.\n'
