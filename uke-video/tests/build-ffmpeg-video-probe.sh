#!/usr/bin/env bash
# Host emulation only. No video node is mounted or opened.
set -Eeuo pipefail
cd /build
[[ $(uname -m) == aarch64 && $(gcc -dumpmachine) == aarch64-redhat-linux ]]
[[ ! -e BINARIES-SHA256SUMS ]]
{ uname -m; gcc -dumpmachine; rpm -q gcc glibc make; } > environment.txt
/source/configure --disable-autodetect --disable-doc --disable-debug --disable-network \
 --disable-shared --enable-static --disable-everything --enable-pthreads --enable-v4l2-m2m \
 --disable-programs --enable-ffmpeg --enable-ffprobe \
 --enable-decoder=h264_v4l2m2m,hevc_v4l2m2m,vp9_v4l2m2m,rawvideo \
 --enable-encoder=h264_v4l2m2m,hevc_v4l2m2m,rawvideo,wrapped_avframe \
 --enable-parser=h264,hevc,vp9 \
 --enable-demuxer=h264,hevc,ivf,matroska,mov,rawvideo \
 --enable-muxer=framehash,framecrc,rawvideo,h264,hevc,null \
 --enable-protocol=file,pipe --enable-filter=null,format,scale,setparams,showinfo \
 --enable-swscale > configure.log 2>&1
for macro in CONFIG_H264_V4L2M2M_DECODER CONFIG_HEVC_V4L2M2M_DECODER CONFIG_VP9_V4L2M2M_DECODER CONFIG_H264_V4L2M2M_ENCODER CONFIG_HEVC_V4L2M2M_ENCODER; do
    grep -Fxq "#define $macro 1" config_components.h
done
for macro in CONFIG_WRAPPED_AVFRAME_ENCODER CONFIG_SETPARAMS_FILTER CONFIG_SHOWINFO_FILTER; do
    grep -Fxq "#define $macro 1" config_components.h
done
grep -Fxq '#define CONFIG_V4L2_M2M 1' config.h
make -j"$(nproc)" ffmpeg ffprobe > build.log 2>&1
mv ffmpeg uke-ffmpeg-video-probe
mv ffprobe uke-ffprobe-video-probe
./uke-ffmpeg-video-probe -version > version.txt 2>&1
./uke-ffmpeg-video-probe -hide_banner -decoders > decoders.txt 2>&1
./uke-ffmpeg-video-probe -hide_banner -encoders > encoders.txt 2>&1
for decoder in h264_v4l2m2m hevc_v4l2m2m vp9_v4l2m2m; do
    grep -Eq "[[:space:]]$decoder[[:space:]]" decoders.txt
done
for encoder in h264_v4l2m2m hevc_v4l2m2m rawvideo wrapped_avframe; do
    grep -Eq "[[:space:]]$encoder[[:space:]]" encoders.txt
done
./uke-ffmpeg-video-probe -hide_banner -filters > filters.txt 2>&1
for filter in setparams showinfo; do
    grep -Eq "[[:space:]]$filter[[:space:]]" filters.txt
done
if grep -Eq '[[:space:]](h264|hevc|vp9)[[:space:]]' decoders.txt; then
    printf 'Unexpected software compressed-video decoder\n' >&2; exit 1
fi
file uke-ffmpeg-video-probe uke-ffprobe-video-probe > elf.txt
readelf -d uke-ffmpeg-video-probe > dynamic.txt
readelf --version-info uke-ffmpeg-video-probe > symbol-versions.txt
sha256sum uke-ffmpeg-video-probe uke-ffprobe-video-probe > BINARIES-SHA256SUMS
printf '%s\n' 'PASS: emulated AArch64 diagnostic CLI build and wrapper inventory only; no device codec acceptance.'
