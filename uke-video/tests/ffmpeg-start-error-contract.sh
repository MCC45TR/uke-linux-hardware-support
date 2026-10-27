#!/usr/bin/env bash
# Run the actual FFmpeg start/receive functions with controlled device failures.
set -Eeuo pipefail
source_root=$(realpath "${1:?Pass the prepared FFmpeg source root}")
work=$(mktemp -d)
trap 'for file in "$work/check.cpp" "$work/check"; do if test -f "$file"; then rm -- "$file"; fi; done; rmdir "$work"' EXIT
cat > "$work/check.cpp" <<'CPP'
#include <cerrno>
#include <cstdio>
#include <linux/videodev2.h>
#define AVERROR(e) (-(e))
#define AVERROR_EOF (-12345)
enum { AV_LOG_DEBUG, AV_LOG_WARNING, AV_LOG_ERROR, AV_CODEC_ID_RAWVIDEO };
struct AVPacket { int size = 0; };
struct AVFrame { bool produced = false; };
struct V4L2m2mPriv;
struct AVCodecContext {
    V4L2m2mPriv *priv_data;
    int pix_fmt = 0, coded_height = 240, coded_width = 320;
};
struct V4L2Context {
    int type = 0, streamon = 0, av_pix_fmt = 0, height = 0, width = 0;
    v4l2_format format = {};
    void *buffers = nullptr;
};
struct V4L2m2mContext {
    V4L2Context capture, output;
    int fd = 42, draining = 0, initial_source_change = 0;
    AVPacket buf_pkt;
};
struct V4L2m2mPriv { V4L2m2mContext *context; int reset_error = 0; };
static V4L2m2mContext state;
static int output_error, format_errno, init_error, capture_error, enqueue_error;
static int packet_error, dequeue_calls, last_timeout, format_calls, packet_reads;
static int dequeue_retries, dequeue_error;
static int packet_budget, packet_after_budget, enqueue_retries;
static int failures;
// Reset is covered separately; ordinary startup must not reopen the decoder.
[[maybe_unused]] static int v4l2_decode_init(AVCodecContext *) { ++failures; return AVERROR(EIO); }
[[maybe_unused]] static int ff_v4l2_m2m_codec_end(V4L2m2mPriv *) { ++failures; return AVERROR(EIO); }
template<class... Args> static void av_log(Args...) {}
static int ff_v4l2_format_v4l2_to_avfmt(unsigned, int) { return 17; }
static int test_ioctl(int, unsigned long request, void *) {
    if (request == VIDIOC_G_FMT) {
        ++format_calls;
        if (format_errno) { errno = format_errno; return -1; }
    }
    if (request == VIDIOC_S_SELECTION) { errno = EINVAL; return -1; }
    return 0;
}
#define ioctl test_ioctl
static int ff_v4l2_context_set_status(V4L2Context *ctx, int) {
    const int error = ctx == &state.output ? output_error : capture_error;
    if (!error) ctx->streamon = 1;
    return error;
}
static int ff_v4l2_context_init(V4L2Context *ctx) {
    if (!init_error) ctx->buffers = ctx;
    return init_error;
}
[[maybe_unused]] static int ff_v4l2_context_wait_source_change(V4L2Context *, int) { return 0; }
static int ff_decode_get_packet(AVCodecContext *, AVPacket *packet) {
    ++packet_reads;
    if (!packet_budget) return packet_after_budget;
    if (packet_budget > 0) --packet_budget;
    if (!packet_error) packet->size = 12;
    return packet_error;
}
static int ff_v4l2_context_enqueue_packet(V4L2Context *, AVPacket *packet) {
    if (enqueue_retries) { --enqueue_retries; return AVERROR(EAGAIN); }
    if (!enqueue_error && !packet->size) state.draining = 1;
    return enqueue_error;
}
static void av_packet_unref(AVPacket *packet) { packet->size = 0; }
static int ff_v4l2_context_dequeue_frame(V4L2Context *, AVFrame *frame, int timeout) {
    ++dequeue_calls;
    last_timeout = timeout;
    if (dequeue_retries) { --dequeue_retries; return AVERROR(EAGAIN); }
    if (dequeue_error) return dequeue_error;
    frame->produced = true;
    return 0;
}
CPP
for fn in v4l2_try_start v4l2_receive_frame; do
    awk -v fn="$fn" '$0 ~ "^static int " fn "\\(" {emit=1} emit {print} emit && /^}/ {found=1;exit} END {if(!found)exit 1}' \
        "$source_root/libavcodec/v4l2_m2m_dec.c" >> "$work/check.cpp"
done
cat >> "$work/check.cpp" <<'CPP'
static void expect(bool condition, const char *message) {
    if (!condition) { std::fprintf(stderr, "FAIL: %s\n", message); ++failures; }
}
static void reset() {
    state = {};
    state.capture.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    state.output.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    output_error = format_errno = init_error = capture_error = enqueue_error = 0;
    packet_error = dequeue_calls = format_calls = last_timeout = 0;
    packet_reads = dequeue_retries = dequeue_error = 0;
    packet_budget = -1; packet_after_budget = AVERROR(EAGAIN); enqueue_retries = 0;
}
static int receive(AVFrame &frame) {
    V4L2m2mPriv priv{&state};
    AVCodecContext avctx{&priv};
    return v4l2_receive_frame(&avctx, &frame);
}
static void failed_start(int expected, const char *message) {
    AVFrame frame;
    expect(receive(frame) == expected, message);
    expect(!frame.produced && dequeue_calls == 0,
           "failed startup must not report a frame or dequeue capture");
    expect(state.buf_pkt.size == 0, "fatal startup must release the pending packet");
}
int main() {
    reset(); output_error = AVERROR(EACCES);
    failed_start(AVERROR(EACCES), "output STREAMON denial must reach the caller");
    reset(); format_errno = EBUSY;
    failed_start(AVERROR(EBUSY), "G_FMT must preserve errno rather than return raw -1");
    reset(); init_error = AVERROR(EINVAL);
    failed_start(AVERROR(EINVAL), "capture initialization must preserve its actual error");
    reset(); init_error = AVERROR(ENOMEM);
    failed_start(AVERROR(ENOMEM), "allocation failure must remain ENOMEM");
    reset(); capture_error = AVERROR(EIO);
    failed_start(AVERROR(EIO), "capture STREAMON failure must reach the caller");

    reset(); enqueue_error = AVERROR(EAGAIN); format_errno = EAGAIN;
    AVFrame retry;
    expect(receive(retry) == AVERROR(EAGAIN), "retryable startup must return EAGAIN");
    expect(state.buf_pkt.size == 12 && !retry.produced && dequeue_calls == 0,
           "retryable startup must retain an unqueued packet");

    reset(); AVFrame success;
    expect(receive(success) == 0 && success.produced && dequeue_calls == 1,
           "successful startup must still deliver capture output");
    expect(state.output.streamon && state.capture.streamon && state.buf_pkt.size == 0,
           "successful startup must start both queues and release the queued packet");

    reset(); state.output.streamon = state.capture.streamon = 1;
    enqueue_error = AVERROR(EAGAIN); AVFrame backpressure;
    expect(receive(backpressure) == 0 && backpressure.produced,
           "running capture must still progress when input is full");
    expect(state.buf_pkt.size == 12 && format_calls == 0,
           "running input backpressure must retain the packet and skip capture setup");

    reset(); enqueue_error = AVERROR(EIO); AVFrame input_failure;
    expect(receive(input_failure) == AVERROR(EIO) && state.buf_pkt.size == 0,
           "fatal enqueue failure must still propagate and release its packet");
    reset(); state.output.streamon = state.capture.streamon = 1;
    packet_error = AVERROR(EAGAIN); AVFrame available;
    expect(receive(available) == 0 && available.produced && last_timeout == 0,
           "missing new input must retain the existing nonblocking capture path");

    reset(); state.output.streamon = state.capture.streamon = state.draining = 1;
    dequeue_retries = 2; AVFrame draining;
    expect(receive(draining) == 0 && draining.produced && dequeue_calls == 3 && !packet_reads,
           "flush waits across capture readiness gaps without requesting new input");
    reset(); state.output.streamon = state.capture.streamon = state.draining = 1;
    dequeue_error = AVERROR_EOF; AVFrame ended;
    expect(receive(ended) == AVERROR_EOF && !ended.produced && !packet_reads,
           "completed flush returns EOF without consuming another packet");
    reset(); state.output.streamon = state.capture.streamon = state.draining = 1;
    dequeue_error = AVERROR(EIO); AVFrame failed_drain;
    expect(receive(failed_drain) == AVERROR(EIO) && !failed_drain.produced && dequeue_calls == 1,
           "terminal drain error propagates without retrying");
    reset(); state.output.streamon = state.capture.streamon = 1;
    packet_budget = 1; dequeue_error = AVERROR(EAGAIN); AVFrame normal_gap;
    expect(receive(normal_gap) == AVERROR(EAGAIN) && dequeue_calls == 2 && !normal_gap.produced,
           "actual input exhaustion still returns nonblocking EAGAIN to its caller");
    expect(packet_reads == 2 && last_timeout == 0,
           "readiness retry must check buffered input before requesting more external input");

    reset(); state.output.streamon = state.capture.streamon = 1;
    packet_budget = 2; packet_after_budget = AVERROR_EOF;
    dequeue_retries = 2; AVFrame buffered_flush;
    expect(receive(buffered_flush) == 0 && buffered_flush.produced && state.draining,
           "flush must consume buffered packets and send STOP across capture readiness gaps");
    expect(packet_reads == 3 && dequeue_calls == 3 && state.buf_pkt.size == 0,
           "each buffered packet is consumed exactly once before drain");

    reset(); state.output.streamon = state.capture.streamon = 1;
    packet_budget = 1; packet_after_budget = AVERROR_EOF;
    enqueue_retries = dequeue_retries = 1; AVFrame pending_flush;
    expect(receive(pending_flush) == 0 && pending_flush.produced && state.buf_pkt.size == 0,
           "input backpressure must retry the retained packet before requesting another packet");
    expect(packet_reads == 1 && dequeue_calls == 2,
           "retained packet must survive the intermediate readiness gap without a duplicate read");
    if (failures) return 1;
    std::puts("PASS: actual FFmpeg startup errors, packet lifetime and receive controls (host only)");
}
CPP
# The extracted upstream C uses { 0 } to initialize Linux UAPI structs.
"${CXX:-g++}" -std=c++17 -O2 -Wall -Wextra -Werror -Wno-missing-field-initializers \
    "$work/check.cpp" -o "$work/check"
"$work/check"
