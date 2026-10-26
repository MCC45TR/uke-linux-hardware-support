#!/usr/bin/env bash
# Exercise actual FFmpeg C callbacks against controlled V4L2 event boundaries.
set -Eeuo pipefail
source_root=$(realpath "${1:?Pass the prepared FFmpeg source}")
build_root=$(realpath "${2:?Pass its configured host build with libavutil.a}")
work=$(mktemp -d)
trap 'for file in "$work/check.c" "$work/check"; do if test -f "$file"; then rm -- "$file"; fi; done; rmdir "$work"' EXIT
cat > "$work/check.c" <<'C'
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "libavcodec/v4l2_context.c"
#include "libavcodec/v4l2_m2m_dec.c"
/* Upstream internal.h disables libc assertions; keep fixture checks active. */
#undef NDEBUG
#include <assert.h>

static V4L2m2mContext state;
static AVCodecContext codec;
static V4L2m2mPriv priv;
static V4L2Buffer input[4], capture[4];
static struct v4l2_format next_format;
static int source_events, initial_consumed, subscribe_errno, early_queries;
static int queued_packets, metadata_after, packets_available, packet_reads, packets_delivered;
static int output_release_on_wait, released_output;
static int64_t queued_pts[8], input_pts[4];
static int start_commands, capture_starts, reinitializations, produced;
static int output_done[4], capture_ready, capture_next, last_flags, empty_last;
static unsigned long failed_ioctl;
static int ioctl_errno, event_interrupts, poll_interrupts, poll_calls;
static int forced_poll, extra_revents, input_eof;
static int failures;

static void expect(int ok, const char *message)
{
    if (!ok) { fprintf(stderr, "FAIL: %s\n", message); failures++; }
}

int fixture_ioctl(int fd, unsigned long request, ...)
{
    va_list args;
    void *arg;
    (void)fd;
    va_start(args, request); arg = va_arg(args, void *); va_end(args);
    if (request == failed_ioctl) { errno = ioctl_errno; return -1; }
    if (request == VIDIOC_SUBSCRIBE_EVENT) {
        if (subscribe_errno) { errno = subscribe_errno; return -1; }
        return 0;
    }
    if (request == VIDIOC_DQEVENT) {
        if (event_interrupts) { event_interrupts--; errno = EINTR; return -1; }
        if (!source_events) { errno = EAGAIN; return -1; }
        source_events--;
        memset(arg, 0, sizeof(struct v4l2_event));
        ((struct v4l2_event *)arg)->type = V4L2_EVENT_SOURCE_CHANGE;
        ((struct v4l2_event *)arg)->u.src_change.changes = V4L2_EVENT_SRC_CH_RESOLUTION;
        initial_consumed = 1;
        return 0;
    }
    if (request == VIDIOC_G_FMT) {
        early_queries += !initial_consumed && !subscribe_errno;
        *(struct v4l2_format *)arg = next_format;
        return 0;
    }
    if (request == VIDIOC_S_SELECTION || request == VIDIOC_CROPCAP) {
        errno = EINVAL; return -1;
    }
    if (request == VIDIOC_DECODER_CMD) {
        start_commands += ((struct v4l2_decoder_cmd *)arg)->cmd == V4L2_DEC_CMD_START;
        return 0;
    }
    if (request == VIDIOC_STREAMON || request == VIDIOC_STREAMOFF) {
        if (*(int *)arg == (int)state.capture.type && request == VIDIOC_STREAMON)
            capture_starts++;
        return 0;
    }
    if (request == VIDIOC_DQBUF) {
        struct v4l2_buffer *buf = arg;
        if (V4L2_TYPE_IS_OUTPUT(buf->type)) {
            for (int i = 0; i < 4; i++) if (output_done[i]) {
                output_done[i] = 0; buf->index = i; return 0;
            }
        } else if (capture_ready) {
            buf->index = capture_next++ % 4;
            buf->flags = capture_ready == 1 ? last_flags : 0;
            unsigned used = capture_ready == 1 && empty_last ? 0 : 64;
            if (V4L2_TYPE_IS_MULTIPLANAR(buf->type)) buf->m.planes[0].bytesused = used;
            else buf->bytesused = used;
            capture_ready--;
            return 0;
        }
        errno = EAGAIN; return -1;
    }
    errno = ENOTTY;
    return -1;
}

int fixture_poll(struct pollfd *fds, nfds_t count, int timeout)
{
    assert(count == 1);
    poll_calls++;
    if (poll_interrupts) { poll_interrupts--; errno = EINTR; return -1; }
    if (forced_poll != 2) {
        int result = forced_poll;
        forced_poll = 2;
        fds[0].revents = 0;
        if (result < 0) errno = EIO;
        return result;
    }
    if (output_release_on_wait && timeout < 0) {
        expect(fds[0].events & POLLOUT, "initial metadata wait also observes output space");
        output_release_on_wait = 0;
        output_done[0] = 1;
        released_output++;
    }
    fds[0].revents = source_events ? POLLPRI : 0;
    for (int i = 0; i < 4; i++) if (output_done[i]) fds[0].revents |= POLLOUT;
    if (state.capture.streamon && capture_ready) fds[0].revents |= POLLIN;
    fds[0].revents &= fds[0].events;
    fds[0].revents |= extra_revents;
    return !!fds[0].revents;
}

int ff_v4l2_m2m_codec_reinit(V4L2m2mContext *s)
{
    reinitializations++;
    s->capture.streamon = 0;
    s->capture.format = next_format;
    return 0;
}
/* Lifecycle reset has its own fixture; streaming must not silently reopen. */
int ff_v4l2_m2m_create_context(V4L2m2mPriv *priv, V4L2m2mContext **s)
{
    (void)priv; (void)s;
    expect(0, "unexpected decoder allocation during streaming"); return AVERROR(EIO);
}
int ff_v4l2_m2m_codec_init(V4L2m2mPriv *priv)
{
    (void)priv;
    expect(0, "unexpected decoder initialization during streaming"); return AVERROR(EIO);
}
int ff_v4l2_m2m_codec_end(V4L2m2mPriv *priv)
{
    (void)priv;
    expect(0, "unexpected decoder close during streaming"); return AVERROR(EIO);
}
int ff_set_dimensions(AVCodecContext *avctx, int width, int height)
{
    avctx->width = width; avctx->height = height; return 0;
}
enum AVPixelFormat ff_v4l2_format_v4l2_to_avfmt(uint32_t format, enum AVCodecID id)
{
    (void)format; (void)id; return AV_PIX_FMT_NV12;
}
int ff_v4l2_buffer_initialize(V4L2Buffer *buf, int index)
{
    (void)buf; (void)index; return 0;
}
int ff_v4l2_buffer_avpkt_to_buf(const AVPacket *packet, V4L2Buffer *buf)
{
    buf->buf.bytesused = packet->size;
    input_pts[buf - input] = packet->pts;
    return 0;
}
int ff_v4l2_buffer_avframe_to_buf(const AVFrame *frame, V4L2Buffer *buf)
{
    (void)frame; (void)buf; return 0;
}
int ff_v4l2_buffer_enqueue(V4L2Buffer *buf)
{
    buf->status = V4L2BUF_IN_DRIVER;
    int index = (int)(buf - input);
    assert(index >= 0 && index < 4);
    output_done[index] = 1;
    assert(queued_packets < 8);
    queued_pts[queued_packets] = input_pts[index];
    if (++queued_packets == metadata_after) source_events++;
    return 0;
}
int ff_v4l2_buffer_buf_to_avframe(AVFrame *frame, V4L2Buffer *buf)
{
    (void)buf; frame->width = 320; frame->height = 240; produced++; return 0;
}
int ff_decode_get_packet(AVCodecContext *avctx, AVPacket *packet)
{
    (void)avctx;
    packet_reads++;
    if (packets_delivered >= packets_available) return input_eof ? AVERROR_EOF : AVERROR(EAGAIN);
    packet->size = 32;
    packet->pts = ++packets_delivered;
    return 0;
}
void av_packet_unref(AVPacket *packet) { packet->size = 0; }

static void reset(int multiplanar)
{
    memset(&state, 0, sizeof(state)); memset(&codec, 0, sizeof(codec));
    memset(input, 0, sizeof(input)); memset(capture, 0, sizeof(capture));
    memset(output_done, 0, sizeof(output_done));
    memset(queued_pts, 0, sizeof(queued_pts)); memset(input_pts, 0, sizeof(input_pts));
    source_events = initial_consumed = subscribe_errno = early_queries = 0;
    queued_packets = packet_reads = start_commands = capture_starts = 0;
    packets_delivered = output_release_on_wait = released_output = 0;
    reinitializations = produced = capture_next = last_flags = empty_last = 0;
    failed_ioctl = 0; ioctl_errno = EIO;
    event_interrupts = poll_interrupts = poll_calls = extra_revents = input_eof = 0;
    forced_poll = 2;
    metadata_after = packets_available = 2; capture_ready = 1;
    state.fd = 42; state.avctx = &codec;
    priv.context = &state; codec.priv_data = &priv;
    codec.coded_width = 320; codec.coded_height = 240;
    state.capture.type = multiplanar ? V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE : V4L2_BUF_TYPE_VIDEO_CAPTURE;
    state.output.type = multiplanar ? V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE : V4L2_BUF_TYPE_VIDEO_OUTPUT;
    state.capture.buffers = capture; state.output.buffers = input;
    state.capture.num_buffers = state.output.num_buffers = 4;
    state.output.width = 320; state.output.height = 240;
    memset(&next_format, 0, sizeof(next_format));
    next_format.type = state.capture.type;
    if (multiplanar) {
        next_format.fmt.pix_mp.width = 320; next_format.fmt.pix_mp.height = 240;
        next_format.fmt.pix_mp.pixelformat = V4L2_PIX_FMT_NV12;
    } else {
        next_format.fmt.pix.width = 320; next_format.fmt.pix.height = 240;
        next_format.fmt.pix.pixelformat = V4L2_PIX_FMT_NV12;
    }
    state.capture.format = next_format;
    for (int i = 0; i < 4; i++) {
        input[i].context = &state.output;
        capture[i].context = &state.capture; capture[i].status = V4L2BUF_IN_DRIVER;
    }
    ff_mutex_init(&state.buffer_lock, NULL); ff_cond_init(&state.buffer_cond, NULL);
    expect(v4l2_prepare_decoder(&state) == 0, "supported source-event subscription");
}
static void cleanup(void)
{
    ff_cond_destroy(&state.buffer_cond); ff_mutex_destroy(&state.buffer_lock);
}
int main(void)
{
    av_log_set_level(AV_LOG_QUIET);
    for (int multiplanar = 0; multiplanar <= 1; multiplanar++) {
        reset(multiplanar);
        expect(v4l2_try_start(&codec) == AVERROR(EAGAIN), "capture must wait for initial metadata");
        expect(capture_starts == 0 && early_queries == 0, "no unparsed capture query/start");
        cleanup();

        reset(multiplanar);
        source_events = 1;
        expect(v4l2_try_start(&codec) == 0, "ready initial source event permits capture");
        expect(initial_consumed && !start_commands && !reinitializations,
               "initial event needs neither decoder START nor capture teardown");
        expect(early_queries == 0 && capture_starts == 1, "capture starts after metadata");
        cleanup();

        reset(multiplanar);
        AVFrame frame = { 0 };
        expect(v4l2_receive_frame(&codec, &frame) == 0 && produced == 1,
               "metadata spanning packets eventually produces a frame");
        expect(queued_packets == 2 && initial_consumed && early_queries == 0,
               "feed multiple packets before capture setup");
        cleanup();

        reset(multiplanar); metadata_after = 1; packets_available = 1;
        for (int i = 0; i < 4; i++) input[i].status = V4L2BUF_IN_DRIVER;
        output_release_on_wait = 1;
        AVFrame backpressure = { 0 };
        expect(v4l2_receive_frame(&codec, &backpressure) == 0 && produced == 1,
               "output backpressure before initial metadata eventually produces a frame");
        expect(released_output == 1 && queued_packets == 1 && packets_delivered == 1
               && packet_reads == 1 && queued_pts[0] == 1,
               "retain the exact pending packet across output EAGAIN");
        cleanup();

        reset(multiplanar); packets_available = 1;
        AVFrame partial = { 0 };
        expect(v4l2_receive_frame(&codec, &partial) == AVERROR(EAGAIN)
               && queued_packets == 1 && packets_delivered == 1,
               "partial input returns EAGAIN while retaining submitted metadata");
        expect(!capture_starts && !produced && !early_queries && state.initial_source_change,
               "partial metadata does not start capture or invent a frame");
        packets_available = 2;
        expect(v4l2_receive_frame(&codec, &partial) == 0 && produced == 1,
               "later input resumes metadata parsing without restarting the session");
        expect(queued_packets == 2 && packets_delivered == 2
               && queued_pts[0] == 1 && queued_pts[1] == 2,
               "partial input preserves packet identity and order");
        cleanup();

        reset(multiplanar); subscribe_errno = ENOTTY;
        expect(v4l2_prepare_decoder(&state) == 0 && v4l2_try_start(&codec) == 0,
               "known dimensions retain the legacy path without source events");
        cleanup();
        reset(multiplanar); subscribe_errno = ENOTTY;
        state.output.width = state.output.height = 0;
        expect(v4l2_prepare_decoder(&state) == AVERROR(ENOTTY),
               "missing required metadata support preserves its errno");
        cleanup();

        reset(multiplanar); source_events = 1;
        poll_interrupts = event_interrupts = 1;
        extra_revents = POLLERR;
        expect(v4l2_try_start(&codec) == 0 && initial_consumed && !early_queries,
               "interrupted event/poll and combined PRI/ERR retain valid initial metadata");
        cleanup();

        reset(multiplanar); packets_available = 0; input_eof = 1;
        AVFrame empty = { 0 };
        expect(v4l2_receive_frame(&codec, &empty) == AVERROR_EOF && !capture_starts && !produced,
               "empty input reaches EOF without starting capture or fabricating a frame");
        cleanup();

        for (int changed_size = 0; changed_size <= 1; changed_size++) {
            reset(multiplanar); source_events = 1;
            (void)v4l2_try_start(&codec);
            initial_consumed = 1;
            if (changed_size) {
                if (multiplanar) next_format.fmt.pix_mp.width = 640;
                else next_format.fmt.pix.width = 640;
            }
            source_events = 1; capture_ready = 2; last_flags = V4L2_BUF_FLAG_LAST;
            expect(v4l2_handle_event(&state.capture) == 0,
                   "running source change waits for its boundary");
            expect(!start_commands && !reinitializations,
                   "same-size and pre-first-dequeue changes must not resume or tear down early");
            expect(v4l2_dequeue_v4l2buf(&state.capture, 0) != NULL,
                   "return the old frame before LAST");
            expect(!reinitializations, "retain capture before LAST");
            expect(v4l2_dequeue_v4l2buf(&state.capture, 0) != NULL,
                   "return a nonempty LAST frame before reconfiguration");
            (void)v4l2_dequeue_v4l2buf(&state.capture, 0);
            expect(reinitializations == 1 && !start_commands,
                   "reconfigure only after delivering LAST, including unchanged dimensions");
            cleanup();
        }

        reset(multiplanar); source_events = 1; (void)v4l2_try_start(&codec);
        source_events = 1; (void)v4l2_handle_event(&state.capture);
        capture_ready = 0;
        expect(v4l2_dequeue_v4l2buf(&state.capture, 0) == NULL && !reinitializations,
               "readiness gaps preserve a pending source change");
        capture_ready = 1; last_flags = V4L2_BUF_FLAG_LAST; empty_last = 1;
        expect(v4l2_dequeue_v4l2buf(&state.capture, 0) == NULL && reinitializations == 1,
               "consume an empty LAST without returning an empty frame");
        cleanup();

        reset(multiplanar); source_events = 1; (void)v4l2_try_start(&codec);
        source_events = 1; (void)v4l2_handle_event(&state.capture);
        failed_ioctl = VIDIOC_DQBUF; ioctl_errno = EPIPE;
        AVFrame boundary = { 0 };
        expect(ff_v4l2_context_dequeue_frame(&state.capture, &boundary, 0) == AVERROR(EAGAIN)
               && reinitializations == 1, "EPIPE after a source boundary reconfigures without EOF");
        cleanup();

        for (int failure = 0; failure < 3; failure++) {
            reset(multiplanar); source_events = 1; (void)v4l2_try_start(&codec);
            if (failure == 0) failed_ioctl = VIDIOC_DQBUF;
            if (failure == 1) forced_poll = -1;
            if (failure == 2) { source_events = 1; failed_ioctl = VIDIOC_DQEVENT; }
            AVFrame invalid = { 0 };
            expect(ff_v4l2_context_dequeue_frame(&state.capture, &invalid, 0) == AVERROR(EIO),
                   "device/event/poll errors must not become EOF, EAGAIN or a frame");
            cleanup();
        }

        reset(multiplanar); source_events = 1; (void)v4l2_try_start(&codec);
        source_events = 1; (void)v4l2_handle_event(&state.capture);
        capture_ready = 1; last_flags = V4L2_BUF_FLAG_LAST;
        (void)v4l2_dequeue_v4l2buf(&state.capture, 0);
        failed_ioctl = VIDIOC_G_FMT; ioctl_errno = EBUSY;
        AVFrame failed_format = { 0 };
        expect(ff_v4l2_context_dequeue_frame(&state.capture, &failed_format, 0) == AVERROR(EBUSY),
               "deferred format-query failure preserves its actual errno");
        cleanup();

        reset(multiplanar); source_events = 1; (void)v4l2_try_start(&codec);
        poll_calls = 0; forced_poll = 0; errno = EINTR;
        expect(v4l2_dequeue_v4l2buf(&state.capture, 0) == NULL && poll_calls == 1,
               "a poll timeout must not retry a stale EINTR");
        cleanup();

        reset(multiplanar); failed_ioctl = VIDIOC_DECODER_CMD;
        AVPacket stop = { 0 };
        expect(ff_v4l2_context_enqueue_packet(&state.output, &stop) == AVERROR(EIO) && !state.draining,
               "decoder STOP failure must not report a started drain");
        failed_ioctl = VIDIOC_ENCODER_CMD;
        expect(ff_v4l2_context_enqueue_frame(&state.output, NULL) == AVERROR(EIO) && !state.draining,
               "encoder STOP failure must not report a started drain");
        cleanup();
    }
    if (failures) return 1;
    puts("PASS: actual FFmpeg metadata/backpressure/partial input, LAST/EPIPE boundaries, legacy path and error/STOP handling; both queue APIs (host only)");
    return 0;
}
C
"${CC:-gcc}" -std=c17 -O1 -D_POSIX_C_SOURCE=200809L -DHAVE_AV_CONFIG_H -DBUILDING_avcodec \
    -Dioctl=fixture_ioctl -Dpoll=fixture_poll -ffunction-sections -fdata-sections \
    -I"$build_root" -I"$source_root" -include "$build_root/config.h" \
    "$work/check.c" "$build_root/libavutil/libavutil.a" -Wl,--gc-sections -pthread -lm -o "$work/check"
timeout 10s "$work/check"
