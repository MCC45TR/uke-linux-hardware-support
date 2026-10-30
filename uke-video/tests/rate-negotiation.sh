#!/usr/bin/env bash
# Host-only actual-source ioctl checks; no codec or firmware is exercised.
set -Eeuo pipefail
root=$(realpath "${1:?Pass the prepared backend source}")
work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT
cat > "$work/check.cpp" <<'CPP'
#include "internal.hpp"
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <cstdarg>
#include <cassert>
#include <cerrno>
#include <limits>
using namespace irisva;
namespace contract {
constexpr unsigned OUTPUT = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
constexpr unsigned CAPTURE = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
CPP
for fn in call checked set_bitrate set_frame_rate; do
    awk -v name="$fn" '$0 ~ "^(int|void) " name "\\(" {emit=1}
        emit {print} emit && /^}/ {exit}' "$root/src/encoder.cpp" >> "$work/check.cpp"
done
cat >> "$work/check.cpp" <<'CPP'
}
namespace {
enum class Rate { Q16, Unchanged, Clamped, Zero } rate_mode = Rate::Q16;
int minimum = 64000, maximum = 100000000, step = 1000;
unsigned flags, calls, writes;
unsigned long fail_request;
bool interrupt, change_bitrate;
}
extern "C" int __wrap_ioctl(int fd, unsigned long request, ...) {
    assert(fd == 42); ++calls;
    va_list args; va_start(args,request);
    void *arg=va_arg(args,void *); va_end(args);
    if (interrupt) { interrupt=false; errno=EINTR; return -1; }
    if (request==fail_request) { errno=EIO; return -1; }
    if (request==VIDIOC_QUERYCTRL) {
        auto &range=*static_cast<v4l2_queryctrl *>(arg);
        assert(range.id==V4L2_CID_MPEG_VIDEO_BITRATE ||
               range.id==V4L2_CID_MPEG_VIDEO_BITRATE_PEAK);
        range.type=V4L2_CTRL_TYPE_INTEGER; range.flags=flags;
        range.minimum=minimum; range.maximum=maximum; range.step=step;
        return 0;
    }
    if (request==VIDIOC_S_CTRL) {
        ++writes;
        if (change_bitrate) static_cast<v4l2_control *>(arg)->value/=2;
        return 0;
    }
    assert(request==VIDIOC_S_PARM);
    auto &parm=*static_cast<v4l2_streamparm *>(arg);
    assert(parm.type==contract::CAPTURE || parm.type==contract::OUTPUT);
    auto &rate=parm.type==contract::CAPTURE ? parm.parm.capture.timeperframe
                                          : parm.parm.output.timeperframe;
    assert(rate.numerator && rate.denominator);
    if (uint64_t(rate.denominator)>uint64_t(rate.numerator)*480) {
        errno=ERANGE; return -1;
    }
    if (rate_mode==Rate::Q16) {
        rate.denominator=(uint64_t(rate.denominator)<<16)/rate.numerator;
        rate.numerator=1u<<16;
    } else if (rate_mode==Rate::Clamped) {
        rate.numerator=1; rate.denominator=30;
    } else if (rate_mode==Rate::Zero) rate.numerator=0;
    return 0;
}
template<class F> void rejected(F fn, VAStatus wanted) {
    try { fn(); assert(false); } catch(const Error &e) { assert(e.status==wanted); }
}
int main() {
    using namespace contract;
    for (unsigned type : {CAPTURE,OUTPUT}) {
        for (unsigned fps : {1u,30u,240u,480u}) set_frame_rate(42,type,fps,1);
        set_frame_rate(42,type,30000,1001);
        set_frame_rate(42,type,60000,1001);
        rejected([&]{set_frame_rate(42,type,481,1);},VA_STATUS_ERROR_ENCODING_ERROR);
        rate_mode=Rate::Unchanged; set_frame_rate(42,type,30000,1001);
        rate_mode=Rate::Clamped;
        rejected([&]{set_frame_rate(42,type,60,1);},VA_STATUS_ERROR_ENCODING_ERROR);
        rate_mode=Rate::Zero;
        rejected([&]{set_frame_rate(42,type,30,1);},VA_STATUS_ERROR_ENCODING_ERROR);
        rate_mode=Rate::Q16;
    }
    const auto before=calls;
    for (auto rate : {std::pair{0u,1u},std::pair{30u,0u},std::pair{1u,2u},
                      std::pair{65536u,1u},std::pair{UINT32_MAX,1u}})
        rejected([&]{set_frame_rate(42,OUTPUT,rate.first,rate.second);},
                 VA_STATUS_ERROR_INVALID_PARAMETER);
    assert(calls==before);
    interrupt=true; set_frame_rate(42,OUTPUT,30000,1001);
    for (unsigned id : {V4L2_CID_MPEG_VIDEO_BITRATE,V4L2_CID_MPEG_VIDEO_BITRATE_PEAK}) {
        for (unsigned value : {64000u,10000000u,100000000u}) set_bitrate(42,id,value);
        const auto written=writes;
        for (unsigned value : {0u,63000u,64001u,100001000u,245000000u,UINT32_MAX})
            rejected([&]{set_bitrate(42,id,value);},VA_STATUS_ERROR_INVALID_PARAMETER);
        assert(writes==written);
        for (unsigned flag : {V4L2_CTRL_FLAG_DISABLED,V4L2_CTRL_FLAG_READ_ONLY}) {
            flags=flag;
            rejected([&]{set_bitrate(42,id,10000000);},VA_STATUS_ERROR_UNIMPLEMENTED);
        }
        flags=V4L2_CTRL_FLAG_INACTIVE; set_bitrate(42,id,10000000);
        flags=0; step=0;
        rejected([&]{set_bitrate(42,id,10000000);},VA_STATUS_ERROR_UNIMPLEMENTED);
        step=1000; minimum=-1;
        rejected([&]{set_bitrate(42,id,10000000);},VA_STATUS_ERROR_UNIMPLEMENTED);
        minimum=64000; change_bitrate=true;
        rejected([&]{set_bitrate(42,id,10000000);},VA_STATUS_ERROR_ENCODING_ERROR);
        change_bitrate=false;
        for (unsigned long request : {VIDIOC_QUERYCTRL,VIDIOC_S_CTRL}) {
            fail_request=request;
            rejected([&]{set_bitrate(42,id,10000000);},VA_STATUS_ERROR_ENCODING_ERROR);
        }
        fail_request=0; interrupt=true; set_bitrate(42,id,10000000);
    }
    std::puts("Runtime bitrate bounds, fractional/high FPS and ioctl refusal: PASS (host only)");
}
CPP
g++ -std=c++17 -O2 -Wall -Wextra -Werror $(pkg-config --cflags libva) \
    -I"$root/src" "$work/check.cpp" -Wl,--wrap=ioctl -o "$work/check"
"$work/check"
