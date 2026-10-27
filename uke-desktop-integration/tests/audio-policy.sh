#!/usr/bin/env bash
# Parse actual ALSA data and exercise the route plugin against regular files.
set -Eeuo pipefail
root=$(cd -- "$(dirname -- "$0")/.." && pwd)
conf=$root/src/ucm2/Qualcomm/sm7675
work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT
read -r -a cflags <<< "${ALSA_CFLAGS:-$(pkg-config --cflags alsa)}"
read -r -a libs <<< "${ALSA_LIBS:-$(pkg-config --libs alsa)}"
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror "${cflags[@]}" \
    "$root/tests/audio-policy.cpp" "${libs[@]}" -o "$work/parser"
"$work/parser" "$conf/"*.conf
cat "$conf/DP-ELD.conf" "$conf/Speaker-ELD.conf" > "$work/controls.conf"
"$work/parser" "$work/controls.conf"
cat > "$work/alsa.conf" <<EOF
</usr/share/alsa/alsa.conf>
pcm.!hw {
    @args [ CARD DEV SUBDEV ]
    @args.CARD.type string
    @args.DEV.type integer
    @args.SUBDEV { type integer default -1 }
    type file
    slave.pcm null
    file "$work/output.raw"
    format raw
}
<$conf/Speaker-PCM.conf>
EOF
printf '\043\001\126\004\211\007\274\012' > "$work/input.raw"
printf '\043\001\126\004\043\001\126\004\211\007\274\012\211\007\274\012' > "$work/expected.raw"
ALSA_CONFIG_PATH="$work/alsa.conf" aplay -q -D uke_speakers:fixture \
    -t raw -f S16_LE -r 48000 -c 2 "$work/input.raw"
head -c 16 "$work/output.raw" > "$work/actual.raw"
cmp "$work/expected.raw" "$work/actual.raw"
for ((frame=0; frame<64; frame++)); do
    printf '\000\000\064\022\000\000\170\126'
done > "$work/wide.raw"
ALSA_CONFIG_PATH="$work/alsa.conf" aplay -q -D uke_speakers:fixture \
    -t raw -f S32_LE -r 44100 -c 2 "$work/wide.raw"
od -An -v -w8 -tu2 "$work/output.raw" | awk '
  NF==4 {if ($1!=$3 || $2!=$4) exit 1; if ($1 || $2) signal=1}
  END {if (!signal) exit 1}
'
for prefix in TopLeft TopRight BottomLeft BottomRight; do
    [[ $(grep -c "name='$prefix Speaker Switch' 1" "$conf/Speaker-HiFi.conf") -eq 1 ]]
    [[ $(grep -c "name='$prefix Speaker Switch' 0" "$conf/Speaker-HiFi.conf") -eq 2 ]]
done
grep -Fq 'PlaybackPCM "hw:${CardId},0"' "$conf/Speaker-HiFi.conf"
grep -Fq 'PlaybackPCM "uke_speakers:${CardId}"' "$conf/Speaker-HiFi.conf"
if grep -E 'CapturePCM|CaptureChannels|Digital Volume|force_unmute|max_volume' "$conf/Speaker-"*.conf; then
    echo 'Unqualified capture or gain policy was added' >&2; exit 1
fi
printf 'ALSA syntax, ELD controls, S16/48k and S32/44.1k stereo-to-four-slot conversion passed; host files only\n'
