// SPDX-License-Identifier: MIT
// Bounded public-API diagnostic. A named hardware wrapper is not hardware proof.
#include <array>
#include <charconv>
#include <climits>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <tuple>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/error.h>
#include <libavutil/imgutils.h>
#include <libavutil/mem.h>
#include <libavutil/pixdesc.h>
#include <libavutil/sha.h>
}

static void require(bool condition, const char *message)
{
    if (!condition) throw std::runtime_error(message);
}

static void checked(int result, const char *operation)
{
    if (result >= 0) return;
    char error[AV_ERROR_MAX_STRING_SIZE]{};
    av_strerror(result, error, sizeof(error));
    throw std::runtime_error(std::string(operation) + ": " + error);
}

struct Hash {
    std::unique_ptr<AVSHA, decltype(&av_free)> context{av_sha_alloc(), av_free};
    Hash() { require(bool(context), "SHA allocation failed"); checked(av_sha_init(context.get(), 256), "SHA init"); }
    void update(const uint8_t *data, size_t size) { av_sha_update(context.get(), data, size); }
    std::string finish() {
        std::array<uint8_t, 32> digest{};
        av_sha_final(context.get(), digest.data());
        std::string result;
        for (uint8_t byte : digest) {
            result += "0123456789abcdef"[byte >> 4]; result += "0123456789abcdef"[byte & 15];
        }
        return result;
    }
};

static std::string file_hash(const std::filesystem::path &path)
{
    std::ifstream input(path, std::ios::binary);
    require(bool(input), "Cannot read input");
    Hash hash;
    std::array<uint8_t, 65536> bytes{};
    size_t total = 0;
    while (input) {
        input.read(reinterpret_cast<char *>(bytes.data()), bytes.size());
        const auto size = static_cast<size_t>(input.gcount());
        total += size; require(total <= 64 * 1024 * 1024, "Input exceeds 64 MiB");
        hash.update(bytes.data(), size);
    }
    require(input.eof() && !input.bad(), "Input read failed");
    return hash.finish();
}

struct Session {
    AVFormatContext *input = nullptr;
    AVCodecContext *decoder = nullptr;
    AVPacket *packet = nullptr;
    AVFrame *frame = nullptr, *held = nullptr;
    AVDictionary *options = nullptr;
    int stream = -1;
    void close() {
        av_frame_free(&held); av_frame_free(&frame); av_packet_free(&packet);
        avcodec_free_context(&decoder); avformat_close_input(&input); av_dict_free(&options);
    }
    ~Session() { close(); }
};

struct Record {
    int64_t pts, duration;
    int width, height, depth, primaries, transfer, matrix, range;
    size_t bytes;
    std::string format, sha256;
    bool operator==(const Record &other) const {
        return std::tie(pts, duration, width, height, depth, primaries, transfer, matrix, range, bytes, format, sha256) ==
               std::tie(other.pts, other.duration, other.width, other.height, other.depth, other.primaries,
                        other.transfer, other.matrix, other.range, other.bytes, other.format, other.sha256);
    }
};

static Record record(const AVFrame *frame)
{
    std::unique_ptr<AVFrame, void (*)(AVFrame *)> view(av_frame_clone(frame), [](AVFrame *f) { av_frame_free(&f); });
    require(bool(view), "Cannot clone frame for visible-plane copy");
    checked(av_frame_apply_cropping(view.get(), AV_FRAME_CROP_UNALIGNED), "Apply visible crop");
    require(!(view->flags & AV_FRAME_FLAG_CORRUPT) && !view->decode_error_flags, "Decoder returned a corrupt frame");
    const auto format = static_cast<AVPixelFormat>(view->format);
    const auto *description = av_pix_fmt_desc_get(format);
    const char *name = av_get_pix_fmt_name(format);
    require(description && name && !(description->flags & AV_PIX_FMT_FLAG_HWACCEL), "Frame needs a separate hardware download path");
    const int size = av_image_get_buffer_size(format, view->width, view->height, 1);
    require(size > 0 && size <= 64 * 1024 * 1024, "Visible frame allocation exceeds 64 MiB");
    std::vector<uint8_t> pixels(static_cast<size_t>(size));
    checked(av_image_copy_to_buffer(pixels.data(), size, view->data, view->linesize,
                                   format, view->width, view->height, 1), "Copy visible planes");
    Hash hash; hash.update(pixels.data(), pixels.size());
    const auto pts = view->best_effort_timestamp != AV_NOPTS_VALUE ? view->best_effort_timestamp : view->pts;
    require(pts != AV_NOPTS_VALUE, "Decoded frame has no timestamp");
    return {pts, view->duration, view->width, view->height, description->comp[0].depth,
            view->color_primaries, view->color_trc, view->colorspace, view->color_range,
            pixels.size(), name, hash.finish()};
}

static std::vector<Record> decode(Session &session, int expected, const std::filesystem::path &output,
                                  const std::string &source_hash, const std::string &decoder_name, bool retain)
{
    std::ofstream file(output);
    require(bool(file), "Cannot create frame record");
    const auto time_base = session.input->streams[session.stream]->time_base;
    file << "# source_sha256 " << source_hash << "\n# decoder " << decoder_name
         << "\n# time_base " << time_base.num << '/' << time_base.den
         << "\nindex\tpts\tduration\twidth\theight\tformat\tdepth\tbytes\tsha256\tprimaries\ttransfer\tmatrix\trange\n";
    std::vector<Record> frames;
    auto receive = [&]() {
        for (;;) {
            const int ret = avcodec_receive_frame(session.decoder, session.frame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) return ret;
            checked(ret, "Receive frame");
            require(frames.size() < static_cast<size_t>(expected), "Decoder produced extra frames");
            if (retain && frames.empty()) {
                session.held = av_frame_clone(session.frame);
                require(session.held, "Cannot retain first frame");
            }
            const auto frame = record(session.frame);
            file << frames.size() << '\t' << frame.pts << '\t' << frame.duration << '\t' << frame.width
                 << '\t' << frame.height << '\t' << frame.format << '\t' << frame.depth << '\t' << frame.bytes
                 << '\t' << frame.sha256 << '\t' << frame.primaries << '\t' << frame.transfer
                 << '\t' << frame.matrix << '\t' << frame.range << '\n';
            frames.push_back(frame); av_frame_unref(session.frame);
        }
    };
    auto send = [&](const AVPacket *packet) {
        for (;;) {
            const int ret = avcodec_send_packet(session.decoder, packet);
            if (ret != AVERROR(EAGAIN)) { checked(ret, "Send packet"); return; }
            const size_t before = frames.size();
            require(receive() != AVERROR_EOF && frames.size() > before,
                    "Decoder send/receive made no progress");
        }
    };
    for (;;) {
        const int ret = av_read_frame(session.input, session.packet);
        if (ret == AVERROR_EOF) break;
        checked(ret, "Read packet");
        if (session.packet->stream_index == session.stream) {
            send(session.packet);
            require(receive() != AVERROR_EOF, "Decoder reached EOF before input ended");
        }
        av_packet_unref(session.packet);
    }
    send(nullptr);
    require(receive() == AVERROR_EOF, "Decoder requested input after drain");
    require(frames.size() == static_cast<size_t>(expected), "Decoded frame count differs from expected");
    file.flush(); require(bool(file), "Cannot finish frame record");
    return frames;
}

int main(int argc, char **argv)
{
    try {
        require(argc == 5, "Usage: CODEC INPUT EXPECTED_FRAMES NEW_OUTPUT_DIRECTORY");
        const std::string decoder_name = argv[1];
        const bool software_control = decoder_name == "rawvideo";
        require(software_control || decoder_name == "h264_v4l2m2m" || decoder_name == "hevc_v4l2m2m" ||
                decoder_name == "vp9_v4l2m2m", "Use a named admitted V4L2 wrapper or explicit rawvideo control");
        int expected = 0;
        const std::string count = argv[3];
        const auto parsed = std::from_chars(count.data(), count.data() + count.size(), expected);
        require(parsed.ec == std::errc() && parsed.ptr == count.data() + count.size() && expected > 0 && expected <= 60,
                "Expected frames must be 1..60");
        const auto input_path = std::filesystem::canonical(argv[2]);
        require(std::filesystem::is_regular_file(input_path) && std::filesystem::file_size(input_path) > 0 &&
                std::filesystem::file_size(input_path) <= 64 * 1024 * 1024, "Use a nonempty regular input within 64 MiB");
        const auto source_hash = file_hash(input_path);
        const std::filesystem::path output = argv[4];
        require(mkdir(output.c_str(), 0700) == 0, "Output directory must be new");
        Session session;
        session.input = avformat_alloc_context(); session.packet = av_packet_alloc(); session.frame = av_frame_alloc();
        require(session.input && session.packet && session.frame, "FFmpeg allocation failed");
        checked(av_dict_set(&session.options, "protocol_whitelist", "file", 0), "Limit input protocol");
        const AVInputFormat *format = nullptr;
        if (software_control) {
            format = av_find_input_format("rawvideo"); require(format, "Rawvideo demuxer unavailable");
            checked(av_dict_set(&session.options, "video_size", "320x240", 0), "Set software-control dimensions");
            checked(av_dict_set(&session.options, "pixel_format", "nv12", 0), "Set software-control pixel format");
            checked(av_dict_set(&session.options, "framerate", "30", 0), "Set software-control timing");
        }
        checked(avformat_open_input(&session.input, input_path.c_str(), format, &session.options), "Open input");
        require(!av_dict_count(session.options), "Unconsumed demuxer option");
        // Header metadata suffices for reviewed containers; avoid an extra decoder probe.
        for (unsigned i = 0; i < session.input->nb_streams; ++i)
            if (session.input->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                require(session.stream == -1, "Select an input with exactly one video stream"); session.stream = static_cast<int>(i);
            }
        require(session.stream >= 0, "Input has no video stream");
        const AVCodec *decoder = avcodec_find_decoder_by_name(decoder_name.c_str());
        require(decoder && (software_control || (decoder->capabilities & AV_CODEC_CAP_HARDWARE)), "Named decoder unavailable");
        const AVStream *stream = session.input->streams[session.stream];
        require(stream->codecpar->codec_id == decoder->id && stream->time_base.num > 0 && stream->time_base.den > 0,
                "Input codec or time base differs from the requested path");
        session.decoder = avcodec_alloc_context3(decoder); require(session.decoder, "Cannot allocate decoder");
        checked(avcodec_parameters_to_context(session.decoder, stream->codecpar), "Set codec parameters");
        session.decoder->pkt_timebase = stream->time_base;
        checked(avcodec_open2(session.decoder, decoder, nullptr), "Open named decoder");
        AVCodecContext *const original = session.decoder;
        const auto first = decode(session, expected, output / "first.tsv", source_hash, decoder_name, true);
        const auto held = record(session.held);
        const auto start = stream->start_time == AV_NOPTS_VALUE ? 0 : stream->start_time;
        checked(avformat_seek_file(session.input, session.stream, INT64_MIN, start, start, 0), "Seek demuxer");
        avcodec_flush_buffers(session.decoder);
        require(session.decoder == original && record(session.held) == held, "Flush changed caller context or held frame");
        const auto second = decode(session, expected, output / "second.tsv", source_hash, decoder_name, false);
        require(first == second, "Post-seek frame pixels, timestamps or metadata differ");
        require(session.decoder == original && record(session.held) == held, "Second pass changed caller context or held frame");
        require(file_hash(input_path) == source_hash, "Input changed during test");
        session.close();
        std::ostringstream report;
        report << "{\n  \"functionalPassed\": true,\n  \"hardwareAccepted\": false,\n"
               << "  \"independentReferenceAccepted\": false,\n  \"lifecycleAccepted\": false,\n  \"sameAVCodecContext\": true,\n"
               << "  \"heldFrameUnchanged\": true,\n  \"decoder\": \"" << decoder_name
               << "\",\n  \"softwareControl\": " << (software_control ? "true" : "false")
               << ",\n  \"sourceSHA256\": \"" << source_hash << "\",\n  \"framesPerPass\": " << expected
               << ",\n  \"codecVersion\": " << avcodec_version() << ",\n  \"formatVersion\": " << avformat_version()
               << ",\n  \"copyPath\": \"CPU packed visible planes; no zero-copy or FPS claim\"\n}\n";
        std::ofstream file(output / "result.json"); file << report.str(); file.flush();
        require(bool(file), "Cannot finish result record");
        std::cout << report.str();
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        std::cout << "{\"functionalPassed\":false,\"hardwareAccepted\":false}\n";
        return 1;
    }
}
