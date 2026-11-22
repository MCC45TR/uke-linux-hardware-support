#!/usr/bin/env bash
# Host-only actual-source format/profile checks and P010 plane preservation.
set -Eeuo pipefail
root=$(realpath "${1:?Pass the prepared Main10 backend source}")
work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT
cat > "$work/check.cpp" <<'CPP'
#include "internal.hpp"
#include <array>
#include <cassert>
#include <cstdarg>
#include <linux/dma-buf.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cerrno>
using namespace irisva;
namespace contract {
CPP
for fn in encode_attribute profile_format; do
    awk -v name="$fn" '$0 ~ "^static unsigned " name "\\(" {emit=1}
        emit {print} emit && /^}/ {exit}' "$root/src/backend.cpp" >> "$work/check.cpp"
done
for fn in deblocking validate_h264 validate_hevc validate; do
    awk -v name="$fn" '$0 ~ "^(void|std::array<int, 3>) " name "\\(" {emit=1}
        emit {print} emit && /^}/ {exit}' "$root/src/encoder.cpp" >> "$work/check.cpp"
done
printf '}\nnamespace irisva {\n' >> "$work/check.cpp"
for fn in 'Memory::~Memory' 'Memory::map'; do
    awk -v name="$fn" 'index($0,name "(") && $0 ~ /^(Memory|uint8_t)/ {emit=1}
        emit {print} emit && /^}/ {exit}' "$root/src/decoder.cpp" >> "$work/check.cpp"
done
cat >> "$work/check.cpp" <<'CPP'
}
extern "C" int irisva_msm_allocate(int, uint64_t size, int *coherent) {
    *coherent=1;
    int fd=memfd_create("main10-contract", MFD_CLOEXEC);
    assert(fd >= 0 && !ftruncate(fd, size));
    return fd;
}
extern "C" int irisva_msm_allocate_wc(int fd, uint64_t size) {
    int coherent;
    return irisva_msm_allocate(fd, size, &coherent);
}
extern "C" int __wrap_ioctl(int fd, unsigned long request, ...) {
    assert(fd >= 0 && request == DMA_BUF_IOCTL_SYNC);
    va_list args; va_start(args,request);
    auto sync=va_arg(args,dma_buf_sync *); va_end(args);
    assert(!(sync->flags & ~(DMA_BUF_SYNC_RW | DMA_BUF_SYNC_END)));
    return 0;
}
template<class F> void rejected(F fn, VAStatus wanted) {
    try { fn(); assert(false); } catch(const Error &e) { assert(e.status == wanted); }
}
int main() {
    using namespace contract;
    assert(profile_format(VAProfileHEVCMain10,VAEntrypointEncSlice)==VA_RT_FORMAT_YUV420_10);
    assert(profile_format(VAProfileHEVCMain10,VAEntrypointVLD)==
           (VA_RT_FORMAT_YUV420 | VA_RT_FORMAT_YUV420_10));
    assert(profile_format(VAProfileVP9Profile2,VAEntrypointVLD)==VA_RT_FORMAT_YUV420_10);
    assert(profile_format(VAProfileHEVCMain,VAEntrypointEncSlice)==VA_RT_FORMAT_YUV420);
#if VA_CHECK_VERSION(1, 13, 0)
    assert(encode_attribute(VAConfigAttribEncHEVCFeatures,VAProfileHEVCMain10)==
           encode_attribute(VAConfigAttribEncHEVCFeatures,VAProfileHEVCMain));
    assert(encode_attribute(VAConfigAttribEncHEVCBlockSizes,VAProfileHEVCMain10)!=
           VA_ATTRIB_NOT_SUPPORTED);
#endif
    EncodeSettings s; EncodePicture p;
    s.hevc=s.has_sequence=p.has_params=true;
    auto &seq=s.hevc_sequence;
    seq.general_profile_idc=2; seq.seq_fields.bits.chroma_format_idc=1;
    seq.seq_fields.bits.bit_depth_luma_minus8=seq.seq_fields.bits.bit_depth_chroma_minus8=2;
    seq.pic_width_in_luma_samples=320; seq.pic_height_in_luma_samples=240;
    p.hevc_params.pic_fields.bits.coding_type=1;
    p.hevc_slices.emplace_back();
    p.hevc_slices[0].slice_type=2; p.hevc_slices[0].num_ctu_in_slice=40*30;
    validate(s,p,VAProfileHEVCMain10,320,240);
    s.fps_num=480;
    validate(s,p,VAProfileHEVCMain10,320,240);
    s.fps_num=30;
    rejected([&]{validate(s,p,VAProfileHEVCMain,320,240);},VA_STATUS_ERROR_UNIMPLEMENTED);
    rejected([&]{validate(s,p,VAProfileH264High,320,240);},VA_STATUS_ERROR_INVALID_PARAMETER);
    seq.seq_fields.bits.bit_depth_chroma_minus8=0;
    rejected([&]{validate(s,p,VAProfileHEVCMain10,320,240);},VA_STATUS_ERROR_UNIMPLEMENTED);
    seq.seq_fields.bits.bit_depth_chroma_minus8=2; seq.general_profile_idc=1;
    rejected([&]{validate(s,p,VAProfileHEVCMain10,320,240);},VA_STATUS_ERROR_UNIMPLEMENTED);
    seq.seq_fields.bits.bit_depth_luma_minus8=seq.seq_fields.bits.bit_depth_chroma_minus8=0;
    validate(s,p,VAProfileHEVCMain,320,240);
    seq.pic_width_in_luma_samples=319;
    rejected([&]{validate(s,p,VAProfileHEVCMain,320,240);},VA_STATUS_ERROR_INVALID_PARAMETER);
    auto src=allocate_surface(-1,320,240,VA_FOURCC_P010);
    auto dst=allocate_surface(-1,384,256,VA_FOURCC_P010);
    auto a=src->map(); auto b=dst->map();
    std::memset(a,0x33,src->size); std::memset(b,0xa5,dst->size);
    for(unsigned plane=0;plane<2;++plane)
        for(unsigned row=0;row<(plane ? 120u : 240u);++row)
            for(unsigned col=0;col<320;++col) {
                uint16_t value=((row*319+col*17+plane)%1024)<<6;
                std::memcpy(a+(plane?src->stride*src->storage_height:0)+
                            row*src->stride+col*2,&value,2);
            }
    copy_surface(*dst,*src,320,240);
    for(unsigned plane=0;plane<2;++plane)
        for(unsigned row=0;row<(plane ? 120u : 240u);++row) {
            const auto ai=(plane?src->stride*src->storage_height:0)+row*src->stride;
            const auto bi=(plane?dst->stride*dst->storage_height:0)+row*dst->stride;
            assert(!std::memcmp(a+ai,b+bi,640));
            for(unsigned col=640;col<dst->stride;++col) assert(b[bi+col]==0xa5);
        }
    dst->fourcc=VA_FOURCC_NV12;
    rejected([&]{copy_surface(*dst,*src,320,240);},VA_STATUS_ERROR_INVALID_IMAGE_FORMAT);
    const auto dst_size=dst->size;
    dst->fourcc=VA_FOURCC_P010; dst->size=1;
    rejected([&]{copy_surface(*dst,*src,320,240);},VA_STATUS_ERROR_INVALID_SURFACE);
    dst->size=dst_size;
    std::puts("Main10 profile/format refusal, decoder regression and padded P010 precision: PASS");
}
CPP
g++ -std=c++17 -O2 -Wall -Wextra -Werror $(pkg-config --cflags libva) \
    -I"$root/src" "$work/check.cpp" "$root/src/surface-copy.cpp" \
    -Wl,--wrap=ioctl -o "$work/check"
"$work/check"
