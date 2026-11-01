// SPDX-License-Identifier: GPL-3.0-or-later
// Host/build-worker fixture only: include the actual private production helper.
#include "libssc-proximity-context-private.h"
#include <cassert>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

static bool tag(std::vector<std::uint8_t> bytes, guint32 expected_field, guint expected_wire) {
    GArray array{reinterpret_cast<gchar *>(bytes.data()), guint(bytes.size())};
    guint32 field = 0; guint wire = 0;
    return proximity_first_tag(&array,&field,&wire) && field == expected_field && wire == expected_wire;
}
static void invalid(std::vector<std::uint8_t> bytes) {
    GArray array{reinterpret_cast<gchar *>(bytes.data()), guint(bytes.size())};
    guint32 field = 0; guint wire = 0;
    assert(!proximity_first_tag(&array,&field,&wire));
}
struct Log {
    std::mutex mutex;
    std::vector<std::string> messages;
};
static void record(const gchar *, GLogLevelFlags level, const gchar *message, gpointer opaque) {
    assert(level == G_LOG_LEVEL_MESSAGE);
    auto &log = *static_cast<Log *>(opaque);
    std::lock_guard guard(log.mutex);
    log.messages.emplace_back(message);
}
int main() {
    assert(tag({0x08},1,0));
    assert(tag({0x09},1,1));
    assert(tag({0x0a},1,2));
    assert(tag({0x0b},1,3)); // Diagnostic shape only; no group body is decoded.
    assert(tag({0x0c},1,4));
    assert(tag({0x0d},1,5)); // A truncated float still has an observable tag.
    assert(tag({0x80,0x01},16,0));
    assert(tag({0xfd,0xff,0xff,0xff,0x0f},536870911,5));
    invalid({}); invalid({0}); invalid({7}); invalid({0x0e}); invalid({0x0f});
    invalid({0x80}); invalid({0x80,0x80,0x80,0x80,0x80});
    invalid({0x80,0x80,0x80,0x80,0x10});
    guint32 field{}; guint wire{};
    assert(!proximity_first_tag(nullptr,&field,&wire));
    GArray absent{nullptr,100};
    assert(!proximity_first_tag(&absent,&field,&wire));

    Log log;
    const auto handler = g_log_set_handler(nullptr,G_LOG_LEVEL_MESSAGE,record,&log);
    // First failure has no valid tag; later failures carry a deliberately private marker.
    proximity_unpack_context(1025,nullptr);
    std::string private_payload = "\x0aPRIVATE-MEASUREMENT";
    GArray bytes{private_payload.data(),guint(private_payload.size())};
    std::vector<std::thread> workers;
    for (unsigned i = 0; i < 4; ++i) workers.emplace_back([&] {
        for (unsigned j = 0; j < 50; ++j) proximity_unpack_context(769,&bytes);
    });
    for (auto &worker : workers) worker.join();
    proximity_unpack_context(1025,&bytes); // The counter stays saturated.
    g_log_remove_handler(nullptr,handler);
    assert(log.messages.size() == 8);
    assert(log.messages[0] == "SSC proximity unpack context: event_id=1025 payload_bytes=0 first_tag=invalid");
    for (std::size_t i = 1; i < log.messages.size(); ++i) {
        const auto expected = "SSC proximity unpack context: event_id=769 payload_bytes=" +
            std::to_string(private_payload.size()) + " first_field=1 wire_type=2";
        assert(log.messages[i] == expected);
        assert(log.messages[i].find("PRIVATE") == std::string::npos);
        assert(log.messages[i].find("SUID") == std::string::npos);
    }
    g_print("Proximity context tag bounds, privacy and concurrent eight-record cap passed\n");
}
