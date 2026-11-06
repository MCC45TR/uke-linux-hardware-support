#!/usr/bin/env bash
# COPR only: retain source-derived Core data and consolidate static integration.
set -Eeuo pipefail
export LC_ALL=C
[[ ! -e /sys/firmware/devicetree/base/model && $(rpm --eval '%{_host_cpu}') == aarch64 ]]
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
workspace=${owner%/*}
output=$(realpath -m "${1:?COPR result directory}")
[[ ! -e $output || (-d $output && -z $(find "$output" -mindepth 1 -maxdepth 1 -print -quit)) ]]
work=$(mktemp -d);trap 'find "$work" -depth -delete' EXIT
mkdir -p "$output" "$work/rpm" "$work/outer" "$work/meta" "$work/core" "$work/leaf" "$work/keydb" "$work/source/uke-core-meta-1.0.7/src" "$work/source/uke-core-meta-1.0.7/speaker-controls"
curl -q -fLsS --max-time 90 https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/fedora-rawhide-aarch64/Packages/u/uke-control-2.8.1-5.3.fc46.src.rpm -o "$work/parent.src.rpm"
printf '%s  %s\n' 1db613da6f78eeaba2264bcf2de704efeceafed2e47bca64d370111aea24b383 "$work/parent.src.rpm" | sha256sum -c --strict -
curl -q -fLsS --max-time 30 https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/pubkey.gpg -o "$work/key.asc"
rpmkeys --dbpath "$work/keydb" --import "$work/key.asc"
rpmkeys --dbpath "$work/keydb" --checksig --verbose "$work/parent.src.rpm" > "$work/signatures"
[[ $(grep -Fc 'key fingerprint: 2204a661ea630a5303500085538797e72a3e4eca: OK' "$work/signatures") == 2 ]]
! grep -Eiq 'NOKEY|BAD|NOT OK' "$work/signatures"
[[ $(rpm -qp --qf '%{NAME}-%{VERSION}-%{RELEASE}' "$work/parent.src.rpm") == uke-control-2.8.1-5.3.fc46 ]]
rpm2cpio "$work/parent.src.rpm" > "$work/parent.cpio"
cpio -t --quiet < "$work/parent.cpio" | awk '/^\/|(^|\/)\.\.($|\/)/{bad=1} END{exit bad}'
(cd "$work/rpm"; cpio -idm --quiet --no-absolute-filenames < "$work/parent.cpio")
extract() { tar -tJf "$1" | awk '/^\/|(^|\/)\.\.($|\/)/{bad=1} END{exit bad}';tar -xJf "$1" -C "$2"; }
extract "$work/rpm/uke-control-2.8.1.tar.xz" "$work/outer"
extract "$work/outer/uke-control-2.8.1/inputs/uke-meta-integration/uke-meta-integration-1.0.0.tar.xz" "$work/meta"
meta=$work/meta/uke-meta-integration-1.0.0
extract "$meta/inputs/uke-core-meta/uke-core-meta-1.0.7.tar.xz" "$work/core"
extract "$work/core/uke-core-meta-1.0.7/inputs/uke-core-meta/uke-core-meta-1.0.7.tar.xz" "$work/leaf"
source=$work/source/uke-core-meta-1.0.7
install -m644 "$meta/core-profile.json" "$source/src/profile.json"
install -m644 "$work/leaf/uke-core-meta-1.0.7/LICENSE" "$source/LICENSE"
install -m644 "$work/leaf/uke-core-meta-1.0.7/README.md" "$source/README.md"
install -m644 "$workspace/uke-desktop-integration/src/uke-speaker-controls.desktop" "$source/speaker-controls/uke-speaker-controls.desktop"
install -m644 "$workspace/uke-desktop-integration/src/kmix/ALSA.POCO-Pad-X1.1.default.xml" "$source/speaker-controls/ALSA.POCO-Pad-X1.1.default.xml"
install -m644 "$workspace/uke-desktop-integration/SPEAKER-CONTROLS.md" "$source/speaker-controls/SPEAKER-CONTROLS.md"
install -m644 "$workspace/uke-desktop-integration/LICENSE" "$source/speaker-controls/LICENSE"
install -m644 "$owner/docs/SPEAKER-CONTROLS-MERGE.md" "$source/SPEAKER-CONTROLS-MERGE.md"
install -m644 "$owner/host/speaker-merge-payload.sha256" "$source/SOURCE-SHA256SUMS"
(cd "$source"; sha256sum -c --strict SOURCE-SHA256SUMS)
# Reuse every explicit requirement, recommendation, conflict and old provide.
awk '/^%package -n uke-core-meta$/{p=1;next} /^%description -n uke-core-meta$/{p=0} p && /^(Requires|Recommends|Conflicts|Provides):/{print}' "$work/rpm/uke-control.spec" | sort > "$work/requirements.before"
awk '/^(Requires|Recommends|Conflicts|Provides):/ && $0!="Provides: uke-speaker-controls = 0.1.0-6" && $0!="Requires: (kmix if plasma-workspace)"{print}' "$owner/packaging/uke-core-meta-speaker-merge.spec" | sort > "$work/requirements.after"
cmp "$work/requirements.before" "$work/requirements.after"
install -m644 "$owner/packaging/uke-core-meta-speaker-merge.spec" "$output/uke-core-meta.spec"
[[ $(rpmspec -q --qf '%{NAME}-%{VERSION}-%{RELEASE}.%{ARCH}' "$output/uke-core-meta.spec") == uke-core-meta-1.0.7-20.fc46.aarch64 ]]
commit=$(git -C "$owner" rev-parse HEAD)
printf 'source_commit=%s\nparent_copr_build=11251074\nparent_signed_srpm_sha256=1db613da6f78eeaba2264bcf2de704efeceafed2e47bca64d370111aea24b383\n' "$commit" > "$source/SOURCE-PROVENANCE"
epoch=$(git -C "$owner" show -s --format=%ct HEAD)
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf "$output/uke-core-meta-1.0.7.tar.xz" -C "$work/source" uke-core-meta-1.0.7
printf 'Only Core20 is emitted; complete Core19.2 dependencies/data and source-identical speaker metadata retained.\n'
