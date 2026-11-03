#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Explicit device test: VA hardware encode, independent OpenH264 decode/pixels.
set -Eeuo pipefail
export LC_ALL=C
output=$(realpath -m "${1:?local diagnostic output directory}")
[[ ! ${IRIS_VAAPI_COPY+x} ]] || {
    printf '%s\n' 'Unset IRIS_VAAPI_COPY to test the installed package default.' >&2
    exit 2
}
command -v ffmpeg >/dev/null
mkdir -p "$output"
journalctl -k -b -n 0 --show-cursor --no-pager | sed -n 's/^-- cursor: //p' > "$output/kernel.cursor"
finish() {
    local status=$?
    trap - EXIT
    journalctl -k -b --after-cursor="$(cat "$output/kernel.cursor")" --no-pager > "$output/kernel.log" || true
    exit "$status"
}
trap finish EXIT
date -u '+%Y-%m-%dT%H:%M:%SZ' > "$output/start.utc"
rpm -q uke-vaapi-v4l2 uke-video-session libavcodec-free openh264 > "$output/packages.txt"
for preset in 320x240:30 640x360:30 1280x720:12; do
    dimensions=${preset%:*}; frames=${preset#*:}
    width=${dimensions%x*}; height=${dimensions#*x}
    prefix="$output/$dimensions"
    timeout 15s ffmpeg -nostdin -hide_banner -loglevel error \
        -f lavfi -i "testsrc2=size=$dimensions:rate=30" -frames:v "$frames" \
        -pix_fmt nv12 -f rawvideo "$prefix-source.nv12"
    IRIS_VAAPI_DEBUG=1 timeout --signal=TERM --kill-after=3s 25s ffmpeg \
        -nostdin -hide_banner -loglevel info -init_hw_device vaapi=va:/dev/dri/renderD128 \
        -filter_hw_device va -f rawvideo -pixel_format nv12 -video_size "$dimensions" \
        -framerate 30 -i "$prefix-source.nv12" -frames:v "$frames" -an -vf hwupload \
        -c:v h264_vaapi -profile:v main -bf 0 -rc_mode CQP -qp 23 \
        -f h264 "$prefix.h264" > "$prefix-encode.log" 2>&1
    grep -Fq 'surface copy selected mode=cpu' "$prefix-encode.log"
    grep -Fq 'encoder input path=staging' "$prefix-encode.log"
    timeout 15s ffmpeg -nostdin -hide_banner -loglevel warning -c:v libopenh264 \
        -i "$prefix.h264" -an -pix_fmt nv12 -f rawvideo "$prefix-decoded.nv12" \
        > "$prefix-decode.log" 2>&1
    expected=$((width * height * 3 / 2 * frames))
    [[ $(stat -c %s "$prefix-source.nv12") == "$expected" ]]
    [[ $(stat -c %s "$prefix-decoded.nv12") == "$expected" ]]
    timeout 15s ffmpeg -nostdin -hide_banner -loglevel info \
        -f rawvideo -pixel_format nv12 -video_size "$dimensions" -framerate 30 \
        -i "$prefix-source.nv12" -f rawvideo -pixel_format nv12 \
        -video_size "$dimensions" -framerate 30 -i "$prefix-decoded.nv12" \
        -lavfi "psnr=stats_file=$prefix-frames.psnr" -frames:v "$frames" \
        -f null - > "$prefix-quality.log" 2>&1
    # Reject black/corrupt frames even if the process and frame count succeed.
    awk -v expected="$frames" -v size="$dimensions" '
        { for(i=1;i<=NF;i++) if($i~/^psnr_avg:/) {
            split($i,a,":"); n++;
            if(a[2]!="inf" && a[2]+0<38) bad++;
            if(a[2]!="inf" && (!seen++ || a[2]+0<min)) min=a[2]+0;
        }}
        END { printf "%s frames=%d minimum_psnr=%.2f bad_frames=%d\n",size,n,min,bad;
              exit(n!=expected || bad>0) }
    ' "$prefix-frames.psnr" | tee -a "$output/summary.txt"
    sha256sum "$prefix-source.nv12" "$prefix.h264" "$prefix-decoded.nv12" >> "$output/artifacts.sha256"
    rm -- "$prefix-source.nv12" "$prefix-decoded.nv12"
done
printf '%s\n' 'PASS: default hardware staging and independently decoded pixels at three sizes.'
