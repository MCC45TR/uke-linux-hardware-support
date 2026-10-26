#!/usr/bin/env bash
# Public-API software control, not a VPU test. Requires the prepared FFmpeg SDK.
set -Eeuo pipefail
export LC_ALL=C
source_root=$(realpath "${1:?Pass the prepared FFmpeg source}")
build_root=$(realpath "${2:?Pass its static-library build}")
work=$(realpath -m "${3:?Pass a new output directory}")
script_root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
probe="$script_root/ffmpeg-seek-session.cpp"
[[ ! -e $work ]]
mkdir -m 700 "$work"
for value in 021 035 053; do
    head -c 115200 /dev/zero | tr '\000' "\\$value" >> "$work/input.nv12"
done
for index in 0 1 2; do
    dd if="$work/input.nv12" bs=115200 skip="$index" count=1 status=none | sha256sum
done > "$work/expected-frame-hashes.txt"
input_sha=$(sha256sum "$work/input.nv12"); input_sha=${input_sha%% *}
[[ $(grep -Fc '        avcodec_flush_buffers(session.decoder);' "$probe") == 1 ]]
[[ $(grep -Fc '        checked(avformat_seek_file(' "$probe") == 1 ]]
cp "$probe" "$work/probe.cpp"
awk '$0!="        avcodec_flush_buffers(session.decoder);"' "$probe" > "$work/no-flush.cpp"
awk 'index($0,"        checked(avformat_seek_file("){print "        (void)start;";next}{print}' "$probe" > "$work/no-seek.cpp"
for variant in probe no-flush no-seek; do
    "${CXX:-g++}" -std=c++17 -O2 -Wall -Wextra -Werror -I"$build_root" -I"$source_root" "$work/$variant.cpp" \
        -Wl,--start-group "$build_root/libavformat/libavformat.a" "$build_root/libavcodec/libavcodec.a" \
        "$build_root/libavutil/libavutil.a" -Wl,--end-group -pthread -lm -o "$work/$variant" \
        > "$work/$variant-build.txt" 2>&1
done
timeout 30s "$work/probe" rawvideo "$work/input.nv12" 3 "$work/positive" \
    > "$work/positive.json" 2> "$work/positive.log"
jq -e --arg sha "$input_sha" '.functionalPassed and .sameAVCodecContext and .heldFrameUnchanged and
    .softwareControl and .decoder=="rawvideo" and .framesPerPass==3 and .sourceSHA256==$sha and
    .hardwareAccepted==false and .independentReferenceAccepted==false and .lifecycleAccepted==false' \
    "$work/positive.json" >/dev/null
cmp "$work/positive.json" "$work/positive/result.json"
cmp "$work/positive/first.tsv" "$work/positive/second.tsv"
for pass in first second; do
    grep -Fxq "# source_sha256 $input_sha" "$work/positive/$pass.tsv"
    grep -Fxq '# time_base 1/30' "$work/positive/$pass.tsv"
    awk -F '\t' 'NR==FNR{split($0,fields," ");expected[++n]=fields[1];next}
        /^#/ || /^index/{next}
        {rows++;if($1!=rows-1||$2!=rows-1||$3!=1||$4!=320||$5!=240||$6!="nv12"||$7!=8||$8!=115200||$9!=expected[rows])bad=1}
        END{exit(bad||rows!=3||n!=3)}' "$work/expected-frame-hashes.txt" "$work/positive/$pass.tsv"
done
expect_failure() {
    local name=$1 result
    shift
    set +e
    timeout 30s "$@" > "$work/$name.json" 2> "$work/$name.log"
    result=$?
    set -e
    [[ $result == 1 ]]
    jq -e '.functionalPassed==false and .hardwareAccepted==false' "$work/$name.json" >/dev/null
}
expect_failure missing-flush "$work/no-flush" rawvideo "$work/input.nv12" 3 "$work/no-flush-output"
grep -Fq 'Send packet' "$work/missing-flush.log"
expect_failure missing-seek "$work/no-seek" rawvideo "$work/input.nv12" 3 "$work/no-seek-output"
grep -Fq 'Decoded frame count differs' "$work/missing-seek.log"
expect_failure wrong-count "$work/probe" rawvideo "$work/input.nv12" 4 "$work/wrong-count-output"
expect_failure existing-directory "$work/probe" rawvideo "$work/input.nv12" 3 "$work/positive"
cmp "$work/positive.json" "$work/positive/result.json"
expect_failure software-compressed "$work/probe" h264 "$work/input.nv12" 3 "$work/disallowed-output"
[[ ! -e $work/disallowed-output && ! -e $work/wrong-count-output/result.json ]]
sha256sum "$probe" "$work/input.nv12" "$work/probe" "$work/no-flush" "$work/no-seek" > "$work/INPUTS-BINARIES-SHA256SUMS"
printf '%s\n' 'PASS: public API reuses one decoder across demux seek/drain/flush; six raw frames and held clone match, both missing-call controls fail; software only.'
