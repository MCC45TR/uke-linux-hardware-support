#!/usr/bin/env bash
# Actual upstream mapping/layout functions; buffer-reference creation is modeled.
set -Eeuo pipefail
export LC_ALL=C
ulimit -c 0
source_root=$(realpath "${1:?Prepared FFmpeg source}")
headers=$(realpath "${2:?Configured FFmpeg build headers}")
work=$(realpath -m "${3:?New fixture output directory}")
[[ ! -e $work ]]
mkdir -m 700 "$work"
awk '
 /^static int v4l2_buffer_buf_to_swframe\(/ {inside=1}
 inside {print}
 inside && /^}/ {exit}
' "$source_root/libavcodec/v4l2_buffers.c" > "$work/function.c"
grep -q '^static int v4l2_buffer_buf_to_swframe' "$work/function.c"
cat > "$work/test.c" <<'C'
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "libavutil/common.h"
#include "libavutil/error.h"
#include "libavcodec/v4l2_context.h"
#include "libavcodec/v4l2_buffers.h"
#include "libavcodec/v4l2_fmt.h"
static AVBufferRef references[2];
static int reference_error, failures;
static int v4l2_buf_to_bufref(V4L2Buffer *buffer, int plane, AVBufferRef **reference)
{
    if (reference_error) return AVERROR(ENOMEM);
    references[plane].data = buffer->plane_info[plane].mm_addr;
    *reference = &references[plane];
    return 0;
}
#include "function.c"
static void expect(int value, const char *name)
{
    if (!value) { fprintf(stderr, "FAIL: %s\n", name); failures++; }
}
static void layout(enum AVPixelFormat format, int planes, int stride)
{
    unsigned char memory[384 * 256 * 3], second[768 * 128];
    V4L2Context context = {0}; V4L2Buffer buffer = {0}; AVFrame frame = {0};
    context.av_pix_fmt = format; context.format.fmt.pix_mp.height = 256;
    buffer.context = &context; buffer.num_planes = planes;
    buffer.plane_info[0].mm_addr = memory;
    buffer.plane_info[0].bytesperline = stride;
    buffer.plane_info[1].mm_addr = second;
    buffer.plane_info[1].bytesperline = stride;
    expect(v4l2_buffer_buf_to_swframe(&frame, &buffer) == 0, "layout returns success");
    expect(frame.format == format && frame.data[0] == memory && frame.linesize[0] == stride,
           "native format, luma pointer and byte stride retained");
    expect(frame.linesize[1] == stride && frame.data[1] ==
           (planes == 1 ? memory + stride * 256 : second),
           "UV uses coded height and byte stride or its independent plane");
    reference_error = 1;
    expect(v4l2_buffer_buf_to_swframe(&frame, &buffer) == AVERROR(ENOMEM),
           "buffer-reference failure propagated");
    reference_error = 0;
}
int main(void)
{
    expect(ff_v4l2_format_v4l2_to_avfmt(V4L2_PIX_FMT_P010, AV_CODEC_ID_RAWVIDEO) == AV_PIX_FMT_P010LE,
           "P010 FourCC maps to P010LE");
    expect(ff_v4l2_format_avfmt_to_v4l2(AV_PIX_FMT_P010LE) == V4L2_PIX_FMT_P010,
           "P010LE maps to the linear P010 FourCC");
    expect(ff_v4l2_format_v4l2_to_avfmt(V4L2_PIX_FMT_NV12, AV_CODEC_ID_RAWVIDEO) == AV_PIX_FMT_NV12,
           "NV12 mapping retained");
    expect(ff_v4l2_format_v4l2_to_avfmt(v4l2_fourcc('T','0','1','0'), AV_CODEC_ID_RAWVIDEO) == AV_PIX_FMT_NONE,
           "tiled T010 is not relabeled linear P010");
    layout(AV_PIX_FMT_P010LE, 1, 768);
    layout(AV_PIX_FMT_P010LE, 2, 768);
    layout(AV_PIX_FMT_NV12, 1, 384);
    layout(AV_PIX_FMT_NV21, 1, 384);
    if (failures) return 1;
    puts("PASS: actual linear P010 mapping and padded/separate-plane layout, NV12/NV21 and error controls; no hardware.");
    return 0;
}
C
"${CC:-cc}" -std=gnu11 -O2 -Wall -Wextra -Werror -Wno-sign-compare -I"$headers" -I"$source_root" \
    "$work/test.c" "$source_root/libavcodec/v4l2_fmt.c" -o "$work/test" > "$work/build.log" 2>&1
"$work/test"
