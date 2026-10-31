// SPDX-License-Identifier: MIT
// Synthetic source for an isolated COPR graph; never shipped or run on a tablet.
#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>
#include <algorithm>
#include <csignal>
#include <cstdint>
#include <cstdlib>
struct Source { pw_main_loop *loop; pw_stream *stream; };
static void process(void *data) {
    auto &source = *static_cast<Source *>(data);
    auto *buffer = pw_stream_dequeue_buffer(source.stream);
    if (!buffer) return;
    auto &plane = buffer->buffer->datas[0];
    if (plane.data && plane.chunk) {
        const auto frames = std::min<uint64_t>(buffer->requested ? buffer->requested : 256, plane.maxsize / (4 * sizeof(float)));
        auto *samples = static_cast<float *>(plane.data);
        for (uint64_t frame = 0; frame < frames; ++frame)
            for (unsigned channel = 0; channel < 4; ++channel) samples[frame * 4 + channel] = float(channel + 1) * .01f;
        plane.chunk->offset = 0; plane.chunk->stride = 4 * sizeof(float);
        plane.chunk->size = uint32_t(frames * 4 * sizeof(float));
        buffer->size = frames;
    }
    pw_stream_queue_buffer(source.stream, buffer);
}
static void stop(void *data, int) { pw_main_loop_quit(static_cast<Source *>(data)->loop); }
int main(int argc, char **argv) {
    pw_init(&argc, &argv);
    Source source{};
    source.loop = pw_main_loop_new(nullptr);
    if (!source.loop) return 1;
    pw_stream_events events{}; events.version = PW_VERSION_STREAM_EVENTS; events.process = process;
    source.stream = pw_stream_new_simple(pw_main_loop_get_loop(source.loop), "Uke synthetic fixture",
        pw_properties_new(PW_KEY_NODE_NAME, "uke.fixture.array", PW_KEY_MEDIA_CLASS, "Audio/Source",
            "device.api", "alsa", "alsa.card_name", "POCO-Pad-X1",
            "alsa.components", "mic:uke-4ch-v4 kernel:7.3.1-106",
            "audio.channels", "4", "audio.position", "[ AUX0 AUX1 AUX2 AUX3 ]",
            "node.pause-on-idle", "true", nullptr), &events, &source);
    if (!source.stream) { pw_main_loop_destroy(source.loop); return 1; }
    uint8_t storage[1024]; auto builder = SPA_POD_BUILDER_INIT(storage, sizeof(storage));
    spa_audio_info_raw format{}; format.format = SPA_AUDIO_FORMAT_F32_LE; format.rate = 48000; format.channels = 4;
    for (unsigned i = 0; i < 4; ++i) format.position[i] = spa_audio_channel(SPA_AUDIO_CHANNEL_AUX0 + i);
    const spa_pod *params[] = {spa_format_audio_raw_build(&builder, SPA_PARAM_EnumFormat, &format)};
    const int result = pw_stream_connect(source.stream, PW_DIRECTION_OUTPUT, PW_ID_ANY, PW_STREAM_FLAG_MAP_BUFFERS, params, 1);
    if (result >= 0) {
        pw_loop_add_signal(pw_main_loop_get_loop(source.loop), SIGINT, stop, &source);
        pw_loop_add_signal(pw_main_loop_get_loop(source.loop), SIGTERM, stop, &source);
        pw_main_loop_run(source.loop);
    }
    pw_stream_destroy(source.stream); pw_main_loop_destroy(source.loop); pw_deinit();
    return result < 0 ? 1 : 0;
}
