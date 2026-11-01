#!/bin/bash
# Build only reviewed declarative packages using official Debian tools.
# No compiler, upstream executable or device helper is invoked.
set -euo pipefail
umask 022
root=$(git -C "$(dirname "$0")" rev-parse --show-toplevel)
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
for package in uke-displayport-audio uke-speaker-controls uke-gpu-defaults uke-core-meta; do
 bash "$root/packaging/ubuntu/prepare-source.sh" "$package" "$work/$package" >/dev/null
 source=$(find "$work/$package" -mindepth 1 -maxdepth 1 -type d)
 (cd "$source"; DEB_BUILD_OPTIONS=nocheck dpkg-buildpackage -b -d -us -uc -a arm64 > "$work/$package.build.log" 2>&1) || { cat "$work/$package.build.log"; exit 1; }
 deb=("$work/$package/"*.deb)
 [[ ${#deb[@]} == 1 ]]
 [[ $(dpkg-deb -f "${deb[0]}" Architecture) == arm64 ]]
 dpkg-deb -x "${deb[0]}" "$work/$package-payload"
 payload=$work/$package-payload
 # These packages must remain data-only, without executables or private state.
 [[ ! -d $payload/usr/bin && ! -d $payload/usr/sbin && ! -d $payload/usr/libexec && ! -d $payload/var ]]
 case "$package" in
  uke-displayport-audio)
   [[ $(find "$payload/usr/share/alsa/ucm2/Qualcomm/sm7675" -type f | wc -l) == 7 ]]
   [[ $(find "$payload/usr/share/alsa/ucm2/conf.d/sm7675" -type l | wc -l) == 2 ]]
   for card in POCO-Pad-X1 POCO-Pad-X1-HDMI; do
    cmp "$source/src/ucm2/Qualcomm/sm7675/$card.conf" "$payload/usr/share/alsa/ucm2/conf.d/sm7675/$card.conf"
   done
   dpkg-deb -f "${deb[0]}" Depends | grep -F 'uke-pipewire-audio-abi-1' ;;
  uke-speaker-controls)
   cmp "$source/src/uke-speaker-controls.desktop" "$payload/usr/share/applications/org.uke.SpeakerControls.desktop"
   cmp "$source/src/kmix/ALSA.POCO-Pad-X1.1.default.xml" "$payload/usr/share/kmix/profiles/ALSA.POCO-Pad-X1.1.default.xml" ;;
  uke-gpu-defaults)
   cmp "$source/src/modules-load.d/uke-gpu-modules.conf" "$payload/usr/lib/modules-load.d/uke-gpu.conf"
   cmp "$source/src/modprobe.d/uke-gpu-options.conf" "$payload/usr/lib/modprobe.d/uke-gpu.conf" ;;
  uke-core-meta)
   jq -e '.distribution=="ubuntu" and .series=="tenacious" and .hardware_tested==false and .stable_admitted==false' "$payload/usr/share/senemos/uke/core/profile.json" >/dev/null
   dpkg-deb -f "${deb[0]}" Depends | grep -F 'uke-control-apt-transactions-abi-1' ;;
 esac
 printf 'PASS: %s data-only ARM64 package built and payload verified; dependencies not installed\n' "$package"
done
