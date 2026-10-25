// SPDX-License-Identifier: MIT
#include "internal.hpp"
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <cstdarg>
#include <cassert>
#include <cerrno>

namespace {
unsigned codec;
bool hevc = true, hevc10 = true, vp9 = true, p010 = true, high = true, disabled, substitute, denied, interrupt,
     encoding;
unsigned closes;
}
extern "C" int __wrap_open(const char *, int, ...) {
    if (denied) { errno = EACCES; return -1; }
    return 42;
}
extern "C" int __wrap_close(int fd) { assert(fd == 42); ++closes; return 0; }
extern "C" int __wrap_ioctl(int fd, unsigned long request, ...) {
    assert(fd == 42);
    va_list args;
    va_start(args, request);
    void *arg = va_arg(args, void *);
    va_end(args);
    if (interrupt) { interrupt = false; errno = EINTR; return -1; }
    if (request == VIDIOC_ENUM_FMT) {
        auto &fmt = *static_cast<v4l2_fmtdesc *>(arg);
        const unsigned raw[] = {V4L2_PIX_FMT_NV12, V4L2_PIX_FMT_P010};
        const unsigned coded[] = {V4L2_PIX_FMT_H264, V4L2_PIX_FMT_HEVC, V4L2_PIX_FMT_VP9};
        const bool output = fmt.type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
        if (output != encoding) {
            if (fmt.index >= 3) { errno = EINVAL; return -1; }
            fmt.pixelformat = coded[fmt.index];
            if ((!hevc && fmt.index == 1) || (!vp9 && fmt.index == 2)) {
                errno = EINVAL; return -1;
            }
        } else {
            if (fmt.index >= (p010 ? 2u : 1u)) { errno = EINVAL; return -1; }
            fmt.pixelformat = raw[fmt.index];
        }
        return 0;
    }
    if (request == VIDIOC_S_FMT) {
        auto &fmt = *static_cast<v4l2_format *>(arg);
        codec = fmt.fmt.pix_mp.pixelformat;
        if (substitute && codec != V4L2_PIX_FMT_H264)
            fmt.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_H264;
        return 0;
    }
    if (request == VIDIOC_QUERYCTRL) {
        auto &ctrl = *static_cast<v4l2_queryctrl *>(arg);
        ctrl.type = V4L2_CTRL_TYPE_MENU;
        ctrl.flags = disabled ? V4L2_CTRL_FLAG_DISABLED : 0;
        return 0;
    }
    if (request == VIDIOC_QUERYMENU) {
        auto &menu = *static_cast<v4l2_querymenu *>(arg);
        if (codec == V4L2_PIX_FMT_HEVC && !hevc10 &&
            menu.index == V4L2_MPEG_VIDEO_HEVC_PROFILE_MAIN_10) {
            errno = EINVAL; return -1;
        }
        if (codec == V4L2_PIX_FMT_H264 && !high &&
            menu.index == V4L2_MPEG_VIDEO_H264_PROFILE_HIGH) {
            errno = EINVAL; return -1;
        }
        return 0;
    }
    assert(false);
    return -1;
}
int main() {
    using namespace irisva;
    assert(probe_profiles("", false).empty() && closes == 0);
    assert(probe_profiles("mock", false).size() == 7 && closes == 1);
    p010 = false;
    assert(probe_profiles("mock", false).size() == 5);
    hevc = vp9 = false;
    assert(probe_profiles("mock", false).size() == 3);
    high = false;
    assert(probe_profiles("mock", false).size() == 2);
    disabled = true;
    assert(probe_profiles("mock", false).empty());
    disabled = false; high = hevc = vp9 = p010 = true; substitute = true;
    assert(probe_profiles("mock", false).size() == 3);
    substitute = false; interrupt = true;
    assert(probe_profiles("mock", false).size() == 7);
    encoding = true;
    const auto encoded = probe_profiles("mock", true);
    assert(encoded.size() == 5);
    assert(std::find(encoded.begin(), encoded.end(), VAProfileHEVCMain10) != encoded.end());
    p010 = false;
    assert(probe_profiles("mock", true).size() == 4);
    p010 = true; hevc10 = false;
    assert(probe_profiles("mock", true).size() == 4);
    hevc10 = true;
    hevc = vp9 = false;
    assert(probe_profiles("mock", true).size() == 3);
    denied = true;
    try { probe_profiles("mock", false); assert(false); }
    catch (const Error &e) { assert(e.status == VA_STATUS_ERROR_OPERATION_FAILED); }
    assert(closes == 11);
}
