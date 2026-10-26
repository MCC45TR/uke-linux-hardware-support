#!/usr/bin/env bash
# Actual FFmpeg flush/close/refcount paths; driver discovery and ioctls are modeled.
set -Eeuo pipefail
source_root=$(realpath "${1:?Pass the prepared FFmpeg source}")
build_root=$(realpath "${2:?Pass a matching configured host build with static libraries}")
work=$(mktemp -d)
trap 'for file in "$work/check.c" "$work/check"; do if test -f "$file"; then rm -- "$file"; fi; done; rmdir "$work"' EXIT
cat > "$work/check.c" <<'C'
#define _GNU_SOURCE
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>
#define logger buffer_logger
#include "libavcodec/v4l2_buffers.c"
#undef logger
#include "libavcodec/v4l2_context.c"
#define ff_v4l2_m2m_codec_init unused_driver_discovery
#include "libavcodec/v4l2_m2m.c"
#undef ff_v4l2_m2m_codec_init
#include "libavcodec/v4l2_m2m_dec.c"
#undef NDEBUG
#include <assert.h>

static int opened, closed, queued, unmapped, stops, configure_error, fail_stop, release_error;
static int multiplanar;

int fixture_close(int fd) { assert(fd == 42); closed++; return 0; }
int fixture_munmap(void *address, size_t size)
{
    int ret = syscall(SYS_munmap, address, size);
    assert(ret == 0); unmapped++; return ret;
}
int fixture_ioctl(int fd, unsigned long request, ...)
{
    va_list args;
    void *arg;
    assert(fd == 42);
    va_start(args, request); arg = va_arg(args, void *); va_end(args);
    if (request == VIDIOC_SUBSCRIBE_EVENT) return 0;
    if (request == VIDIOC_STREAMOFF) {
        int type = *(int *)arg;
        stops++;
        if (fail_stop && (fail_stop == 3 || (V4L2_TYPE_IS_CAPTURE(type) ? 2 : 1) == fail_stop)) {
            errno = fail_stop == 3 && V4L2_TYPE_IS_CAPTURE(type) ? EBUSY : EIO; return -1;
        }
        return 0;
    }
    if (request == VIDIOC_REQBUFS) {
        assert(((struct v4l2_requestbuffers *)arg)->count == 0);
        if (release_error) { errno = EIO; return -1; }
        return 0;
    }
    if (request == VIDIOC_QBUF) { queued++; return 0; }
    errno = ENOTTY; return -1;
}
int ff_v4l2_m2m_codec_init(V4L2m2mPriv *priv)
{
    V4L2m2mContext *s = priv->context;
    if (configure_error) return configure_error;
    opened++;
    s->fd = 42;
    s->capture.type = multiplanar ? V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE : V4L2_BUF_TYPE_VIDEO_CAPTURE;
    s->output.type = multiplanar ? V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE : V4L2_BUF_TYPE_VIDEO_OUTPUT;
    return 0;
}
int ff_decode_get_packet(AVCodecContext *codec, AVPacket *packet)
{
    (void)codec; (void)packet; return AVERROR(EAGAIN);
}

static AVBufferRef *hold_buffer(V4L2m2mContext *s)
{
    V4L2Buffer *pool = av_refstruct_allocz(sizeof(*pool));
    AVBufferRef *reference = NULL, *clone;
    assert(pool);
    s->capture.buffers = pool; s->capture.num_buffers = 1;
    pool->context = &s->capture; pool->buf.type = s->capture.type;
    pool->num_planes = 1; pool->plane_info[0].length = 4096;
    pool->plane_info[0].mm_addr = mmap(NULL, 4096, PROT_READ | PROT_WRITE,
                                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    assert(pool->plane_info[0].mm_addr != MAP_FAILED);
    memset(pool->plane_info[0].mm_addr, 0x57, 4096);
    assert(v4l2_buf_to_bufref(pool, 0, &reference) == 0);
    clone = av_buffer_ref(reference); assert(clone);
    av_buffer_unref(&reference);
    return clone;
}

static void exercise(int layout, int held, int stop_error)
{
    AVCodecContext codec = { .codec_id = AV_CODEC_ID_H264, .coded_width = 320,
                            .coded_height = 240, .pix_fmt = AV_PIX_FMT_NV12 };
    V4L2m2mPriv priv = { .num_capture_buffers = 2, .num_output_buffers = 2 };
    AVFrame *frame = av_frame_alloc();
    AVBufferRef *clone = NULL, *packet = NULL;
    V4L2m2mContext *old;
    codec.priv_data = &priv;
    opened = closed = queued = unmapped = stops = configure_error = fail_stop = release_error = 0;
    multiplanar = layout;
    assert(frame && v4l2_decode_init(&codec) == 0);
    old = priv.context;
    assert(av_new_packet(&old->buf_pkt, 32) == 0);
    packet = av_buffer_ref(old->buf_pkt.buf); assert(packet);
    old->draining = 1;
    old->capture.done = old->output.done = 1;
    old->capture.error = AVERROR(EBUSY);
    old->capture.format_change = 2;
    old->capture.streamon = old->output.streamon = 1;
    if (held) clone = hold_buffer(old);
    fail_stop = stop_error;
    ff_h264_v4l2m2m_decoder.flush(&codec);
    assert(!priv.context && stops == 2 && queued == 0);
    assert(av_buffer_get_ref_count(packet) == 1);
    assert(opened == 1 && closed == !held);
    if (held) {
        assert(clone->data[0] == 0x57 && clone->data[4095] == 0x57);
        assert(!unmapped);
    }
    ff_h264_v4l2m2m_decoder.flush(&codec);
    assert(opened == 1 && stops == 2);
    if (stop_error) {
        assert(v4l2_receive_frame(&codec, frame) == AVERROR(EIO));
        assert(!priv.context && opened == 1);
    } else {
        assert(v4l2_receive_frame(&codec, frame) == AVERROR(EAGAIN));
        assert(priv.context && opened == 2);
        if (held) assert(priv.context != old);
        assert(!priv.context->draining && !priv.context->capture.done &&
               !priv.context->capture.error && !priv.context->capture.format_change);
        assert(priv.context->initial_source_change);
    }
    av_buffer_unref(&clone);
    assert(queued == 0 && unmapped == held);
    assert(v4l2_decode_close(&codec) == 0);
    assert(!priv.context && closed == opened);
    av_buffer_unref(&packet); av_frame_free(&frame);
}
static void reopen_failure(void)
{
    AVCodecContext codec = { .codec_id = AV_CODEC_ID_H264 };
    V4L2m2mPriv priv = { 0 };
    AVFrame *frame = av_frame_alloc();
    codec.priv_data = &priv;
    opened = closed = stops = configure_error = fail_stop = 0;
    assert(frame && v4l2_decode_init(&codec) == 0);
    ff_h264_v4l2m2m_decoder.flush(&codec);
    assert(!priv.context && closed == 1 && !stops);
    configure_error = AVERROR(EACCES);
    assert(v4l2_receive_frame(&codec, frame) == AVERROR(EACCES));
    assert(!priv.context && opened == 1);
    configure_error = 0;
    assert(v4l2_receive_frame(&codec, frame) == AVERROR(EAGAIN));
    assert(opened == 2 && priv.context);
    assert(v4l2_decode_close(&codec) == 0 && closed == 2);
    av_frame_free(&frame);
}
static void late_release(void)
{
    const size_t page = sysconf(_SC_PAGESIZE);
    const size_t size = (sizeof(AVCodecContext) + page - 1) / page * page;
    AVCodecContext *codec = mmap(NULL, size, PROT_READ | PROT_WRITE,
                                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    V4L2m2mPriv priv = { 0 };
    AVBufferRef *reference;
    assert(codec != MAP_FAILED);
    opened = closed = queued = unmapped = stops = configure_error = fail_stop = release_error = 0;
    codec->priv_data = &priv; codec->codec_id = AV_CODEC_ID_H264;
    assert(v4l2_decode_init(codec) == 0);
    reference = hold_buffer(priv.context);
    ff_h264_v4l2m2m_decoder.flush(codec);
    assert(!priv.context && !closed && !unmapped);
    assert(mprotect(codec, size, PROT_NONE) == 0);
    release_error = 1; /* Late cleanup logs must not dereference dead caller metadata. */
    av_buffer_unref(&reference);
    assert(closed == 1 && unmapped == 1 && !queued);
    assert(syscall(SYS_munmap, codec, size) == 0);
    release_error = 0;
}
int main(void)
{
    av_log_set_level(AV_LOG_QUIET);
    if (!ff_h264_v4l2m2m_decoder.flush) {
        fputs("FAIL: V4L2 decoder has no flush callback\n", stderr); return 1;
    }
    assert(ff_hevc_v4l2m2m_decoder.flush && ff_vp9_v4l2m2m_decoder.flush);
    for (int layout = 0; layout < 2; layout++)
        for (int held = 0; held < 2; held++)
            for (int error = 0; error < 4; error++) exercise(layout, held, error);
    reopen_failure();
    late_release();
    puts("PASS: actual FFmpeg flush retirement, held-buffer generations, stop/reopen failures and lazy restart; modeled driver, real mappings/refcounts");
    return 0;
}
C
"${CC:-gcc}" -std=c17 -O1 -D_POSIX_C_SOURCE=200809L -DHAVE_AV_CONFIG_H -DBUILDING_avcodec \
    -Dioctl=fixture_ioctl -Dclose=fixture_close -Dmunmap=fixture_munmap -ffunction-sections -fdata-sections \
    -I"$build_root" -I"$source_root" -include "$build_root/config.h" \
    "$work/check.c" "$build_root/libavcodec/libavcodec.a" "$build_root/libavutil/libavutil.a" \
    -Wl,--gc-sections -pthread -lm -o "$work/check"
timeout 10s "$work/check"
