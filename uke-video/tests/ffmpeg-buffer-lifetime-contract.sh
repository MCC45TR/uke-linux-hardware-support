#!/usr/bin/env bash
# Host-only actual FFmpeg buffer/refstruct paths; V4L2 queue ioctls are modeled.
set -Eeuo pipefail
source_root=$(realpath "${1:?Pass the prepared FFmpeg source}")
build_root=$(realpath "${2:?Pass its configured host build}")
work=$(mktemp -d)
trap 'for file in "$work/check.c" "$work/check"; do if test -f "$file"; then rm -- "$file"; fi; done; rmdir "$work"' EXIT
cat > "$work/check.c" <<'C'
#define _GNU_SOURCE
#include <assert.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>
#define logger buffer_logger
#include "libavcodec/v4l2_buffers.c"
#undef logger
#include "libavcodec/v4l2_context.c"
/* Upstream internal.h disables libc assertions; keep fixture checks active. */
#undef NDEBUG
#include <assert.h>

static atomic_int unmapped, queued, context_freed, pools_freed;

int fixture_munmap(void *address, size_t length)
{
    int ret = syscall(SYS_munmap, address, length);
    assert(ret == 0);
    atomic_fetch_add(&unmapped, 1);
    return ret;
}

int fixture_ioctl(int fd, unsigned long request, ...)
{
    va_list args;
    void *arg;
    assert(fd == 42);
    va_start(args, request); arg = va_arg(args, void *); va_end(args);
    if (request == VIDIOC_QBUF) {
        assert(V4L2_TYPE_IS_CAPTURE(((struct v4l2_buffer *)arg)->type));
        atomic_fetch_add(&queued, 1);
        return 0;
    }
    if (request == VIDIOC_REQBUFS) {
        assert(((struct v4l2_requestbuffers *)arg)->count == 0);
        return 0; /* Model a driver which supports orphaned MMAP buffers. */
    }
    errno = ENOTTY;
    return -1;
}

static void free_context(AVRefStructOpaque opaque, void *object)
{
    V4L2m2mContext *s = object;
    (void)opaque;
    ff_cond_destroy(&s->buffer_cond);
    ff_mutex_destroy(&s->buffer_lock);
    atomic_fetch_add(&context_freed, 1);
}

static void free_pool(AVRefStructOpaque opaque, void *object)
{
    (void)opaque; (void)object;
    atomic_fetch_add(&pools_freed, 1);
}

static V4L2Buffer *allocate_pool(V4L2m2mContext *s, int planes, int pattern)
{
    V4L2Buffer *pool = av_refstruct_alloc_ext(sizeof(*pool), 0, NULL, free_pool);
    assert(pool);
    s->capture.buffers = pool;
    s->capture.num_buffers = 1;
    pool->context = &s->capture;
    pool->buf.type = s->capture.type;
    pool->num_planes = planes;
    pool->status = V4L2BUF_AVAILABLE;
    for (int i = 0; i < planes; i++) {
        pool->plane_info[i].length = 4096;
        pool->plane_info[i].mm_addr = mmap(NULL, 4096, PROT_READ | PROT_WRITE,
                                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        assert(pool->plane_info[i].mm_addr != MAP_FAILED);
        memset(pool->plane_info[i].mm_addr, pattern + i, 4096);
    }
    return pool;
}

struct Release {
    AVBufferRef **reference;
    pthread_barrier_t *barrier;
};

static void *release_plane(void *opaque)
{
    struct Release *release = opaque;
    int ret = pthread_barrier_wait(release->barrier);
    assert(ret == 0 || ret == PTHREAD_BARRIER_SERIAL_THREAD);
    av_buffer_unref(release->reference);
    return NULL;
}

static void exercise(int layout, int scenario)
{
    const int planes = layout == 2 ? 2 : 1;
    AVBufferRef *references[2] = { 0 }, *clones[2] = { 0 };
    AVCodecContext codec = { 0 };
    V4L2m2mContext *s = av_refstruct_alloc_ext(sizeof(*s), 0, NULL, free_context);
    assert(s);
    atomic_store(&unmapped, 0); atomic_store(&queued, 0);
    atomic_store(&context_freed, 0); atomic_store(&pools_freed, 0);
    s->self_ref = s;
    s->avctx = &codec;
    s->fd = 42;
    s->capture.type = layout ? V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE : V4L2_BUF_TYPE_VIDEO_CAPTURE;
    s->capture.streamon = 1;
    assert(ff_mutex_init(&s->buffer_lock, NULL) == 0);
    assert(ff_cond_init(&s->buffer_cond, NULL) == 0);
    V4L2Buffer *old_pool = allocate_pool(s, planes, 0x40);
    for (int i = 0; i < planes; i++) {
        assert(v4l2_buf_to_bufref(old_pool, i, &references[i]) == 0);
        clones[i] = av_buffer_ref(references[i]);
        assert(clones[i]);
    }
    assert(atomic_load(&s->refcount) == 1);
    assert(atomic_load(&old_pool->context_refcount) == (unsigned)planes);
    for (int i = 0; i < planes; i++) av_buffer_unref(&references[i]);
    assert(atomic_load(&queued) == 0 && atomic_load(&unmapped) == 0);

    if (scenario) {
        assert(v4l2_release_buffers(&s->capture) == 0);
        assert(atomic_load(&s->refcount) == 0 && old_pool->orphaned);
        assert(atomic_load(&unmapped) == 0);
        av_refstruct_unref(&s->capture.buffers);
        if (scenario == 1) av_refstruct_unref(&s);
        else allocate_pool(s, planes, 0x70);
        assert(atomic_load(&context_freed) == 0 && atomic_load(&pools_freed) == 0);
    }
    for (int i = 0; i < planes; i++) {
        assert(clones[i]->data[0] == 0x40 + i);
        assert(clones[i]->data[4095] == 0x40 + i);
    }

    pthread_t threads[2];
    struct Release release[2];
    pthread_barrier_t barrier;
    assert(pthread_barrier_init(&barrier, NULL, planes + 1) == 0);
    for (int i = 0; i < planes; i++) {
        release[i] = (struct Release){ &clones[i], &barrier };
        assert(pthread_create(&threads[i], NULL, release_plane, &release[i]) == 0);
    }
    int ret = pthread_barrier_wait(&barrier);
    assert(ret == 0 || ret == PTHREAD_BARRIER_SERIAL_THREAD);
    for (int i = 0; i < planes; i++) assert(pthread_join(threads[i], NULL) == 0);
    assert(pthread_barrier_destroy(&barrier) == 0);

    if (scenario == 1) {
        assert(!s && atomic_load(&context_freed) == 1);
        assert(atomic_load(&pools_freed) == 1 && atomic_load(&unmapped) == planes);
    } else {
        assert(atomic_load(&context_freed) == 0 && atomic_load(&s->refcount) == 0);
        assert(atomic_load(&queued) == (scenario == 0));
        if (scenario == 2) {
            assert(atomic_load(&pools_freed) == 1 && atomic_load(&unmapped) == planes);
            for (int i = 0; i < planes; i++)
                assert(((uint8_t *)s->capture.buffers->plane_info[i].mm_addr)[0] == 0x70 + i);
        }
        assert(v4l2_release_buffers(&s->capture) == 0);
        av_refstruct_unref(&s->capture.buffers);
        av_refstruct_unref(&s);
        assert(atomic_load(&context_freed) == 1);
        assert(atomic_load(&pools_freed) == (scenario == 2 ? 2 : 1));
        assert(atomic_load(&unmapped) == planes * (scenario == 2 ? 2 : 1));
    }
    assert(atomic_load(&queued) == (scenario == 0));
}

int main(void)
{
    av_log_set_level(AV_LOG_QUIET);
    for (int layout = 0; layout < 3; layout++)
        for (int scenario = 0; scenario < 3; scenario++) exercise(layout, scenario);
    puts("PASS: actual FFmpeg cloned-plane references, concurrent final releases, orphaned close and replacement pools; real host MMAP, modeled V4L2 ioctls");
    return 0;
}
C
"${CC:-gcc}" -std=c17 -O1 -D_POSIX_C_SOURCE=200809L -DHAVE_AV_CONFIG_H -DBUILDING_avcodec \
    -Dioctl=fixture_ioctl -Dmunmap=fixture_munmap -ffunction-sections -fdata-sections \
    -I"$build_root" -I"$source_root" -include "$build_root/config.h" \
    "$work/check.c" "$build_root/libavcodec/libavcodec.a" "$build_root/libavutil/libavutil.a" \
    -Wl,--gc-sections -pthread -lm -o "$work/check"
timeout 10s "$work/check"
