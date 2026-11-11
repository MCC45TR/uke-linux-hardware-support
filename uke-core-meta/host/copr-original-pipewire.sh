#!/usr/bin/bash
# COPR-only data update. Preserve the complete signed Core20 payload/selection.
set -Eeuo pipefail
trap 'printf "Core original-PipeWire preparation failed at line %s\n" "$LINENO" >&2' ERR
export LC_ALL=C
[[ ! -e /sys/firmware/devicetree/base/model && $(rpm --eval '%{_host_cpu}') == aarch64 ]]
owner=$(cd -- "$(dirname -- "$0")/.." && pwd)
output=$(realpath -m "${1:?COPR result directory}")
[[ $# == 1 && (! -e $output || (-d $output && -z $(find "$output" -mindepth 1 -maxdepth 1 -print -quit))) ]]
work=$(mktemp -d)
trap 'find "$work" -depth -delete' EXIT
mkdir -p "$output" "$work/parent" "$work/source" "$work/keydb"
# Generated Core20 SRPMs are unsigned. Reuse its qualified producer, which
# authenticates the pinned signed source ancestor and preserves every datum.
bash "$owner/host/copr-speaker-controls-merge.sh" "$work/parent"
tar -xJf "$work/parent/uke-core-meta-1.0.7.tar.xz" -C "$work/source"
source=$work/source/uke-core-meta-1.0.7
(cd "$source";sha256sum -c --strict SOURCE-SHA256SUMS)
(cd "$source";find . -type f -print0 | sort -z | xargs -0 sha256sum) > "$work/retained.sha256"
install -m644 "$owner/docs/ORIGINAL-PIPEWIRE.md" "$source/ORIGINAL-PIPEWIRE.md"
cat > "$work/runtime" <<'LIST'
pipewire
pipewire-alsa
pipewire-libs
pipewire-pulseaudio
pipewire-utils
pipewire-plugin-libcamera
pipewire-jack-audio-connection-kit
pipewire-jack-audio-connection-kit-libs
LIST
cat > "$work/optional" <<'LIST'
pipewire-config-raop
pipewire-config-rates
pipewire-config-upmix
pipewire-devel
pipewire-doc
pipewire-gstreamer
pipewire-jack-audio-connection-kit-devel
pipewire-module-ffado
pipewire-module-filter-chain-lv2
pipewire-module-filter-chain-onnx
pipewire-module-filter-chain-sofa
pipewire-module-roc
pipewire-module-x11
pipewire-plugin-jack
pipewire-plugin-vulkan
pipewire-v4l2
LIST
awk '
 FNR==NR{runtime[++n]=$0;next}
 /^Release: 20%\{\?dist\}$/{$0="Release: 21%{?dist}";release++}
 $0=="Requires: uke-pipewire uke-pipewire-alsa wireplumber alsa-utils" {
   print "Requires: wireplumber alsa-utils"
   for(i=1;i<=n;i++)print "Requires: "runtime[i]" >= 1.6.10"
   replaced++;next
 }
 $0=="%doc README.md SPEAKER-CONTROLS-MERGE.md"{$0=$0" ORIGINAL-PIPEWIRE.md";doc++}
 /^%changelog$/ {
   print;print "* Sun Nov 08 2026 Uke contributors - 1.0.7-21"
   print "- Return the complete existing audio runtime to original Fedora PipeWire."
   print "- Preserve Core20, speaker metadata, user policy and boot requirements."
   print "";history++;next
 }
 /^%description$/ {
   for(i=1;i<=n;i++)print "Obsoletes: uke-"runtime[i]" <= 1.6.10-2.fc46"
   print "Provides: uke-core-meta(original-pipewire-v1) = 1"
   additions++;print;next
 }
 {print}END{if(n!=8||release!=1||replaced!=1||doc!=1||history!=1||additions!=1)exit 1}
' "$work/runtime" "$work/parent/uke-core-meta.spec" > "$work/runtime.spec"
# Optional fork packages must be explicitly mapped in the same DNF transaction;
# never obsolete them silently and remove an optional capability.
awk '
 FNR==NR{optional[++n]=$0;next}
 /^%description$/ {for(i=1;i<=n;i++)print "Conflicts: uke-"optional[i]" <= 1.6.10-2.fc46";added++;print;next}
 {print}END{if(n!=16||added!=1)exit 1}
' "$work/optional" "$work/runtime.spec" > "$output/uke-core-meta.spec"
awk '/^(Requires|Recommends|Conflicts|Provides|Obsoletes):/{print}' "$work/parent/uke-core-meta.spec" | grep -Fxv 'Requires: uke-pipewire uke-pipewire-alsa wireplumber alsa-utils' | sort > "$work/deps.before"
awk '/^(Requires|Recommends|Conflicts|Provides|Obsoletes):/{print}' "$output/uke-core-meta.spec" \
 | grep -Ev '^(Requires: (pipewire[^ ]* >= 1[.]6[.]10|wireplumber alsa-utils)|Obsoletes: uke-pipewire[^ ]* <= 1[.]6[.]10-2[.]fc46|Conflicts: uke-pipewire[^ ]* <= 1[.]6[.]10-2[.]fc46|Provides: uke-core-meta\(original-pipewire-v1\) = 1)$' \
 | sort > "$work/deps.after"
cmp "$work/deps.before" "$work/deps.after"
(cd "$source";sha256sum -c --strict --quiet "$work/retained.sha256")
[[ $(rpmspec -q --qf '%{NAME}-%{VERSION}-%{RELEASE}.%{ARCH}' "$output/uke-core-meta.spec") == uke-core-meta-1.0.7-21.fc46.aarch64 ]]
epoch=$(git -C "$owner" show -s --format=%ct HEAD)
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -cJf "$output/uke-core-meta-1.0.7.tar.xz" -C "$work/source" uke-core-meta-1.0.7
printf 'Core21 original PipeWire selection: all Core20 payload/dependency bytes retained except the reviewed eight-runtime return; optional fork removal requires explicit mapping.\n'
