// SPDX-License-Identifier: MIT
// Bounded physical samples; no factory tests, calibration writes or persistent client.
#define UKE_SSC_TEST
#include "ssc-discovery.cpp"
#include <limits>
struct Sample { std::uint64_t timestamp, status; std::vector<float> values; };
static std::vector<Sample> samples(View event, Uid sensor, unsigned dimensions) {
    auto root = fields(event), uid = fields(one(root, 1, 2));
    require(Uid{little(one(uid, 1, 1)), little(one(uid, 2, 1))} == sensor, "Foreign sample sensor");
    std::vector<Sample> result;
    for (auto &entry : root) if (entry.number == 2) {
        require(entry.wire == 2, "Invalid sample event");
        auto e = fields(entry.data); auto id = little(one(e, 1, 5));
        require(id != 130, "SSC returned a sample error");
        if (id != 1025) continue;
        auto data = fields(one(e, 3, 2)); auto status = one(data, 2, 0);
        std::size_t p = 0;
        Sample sample{little(one(e, 2, 1)), varint(status, p), {}};
        require(sample.status <= 3 && sample.timestamp != 0, "Invalid sample metadata");
        require(result.size() < 32 && (result.empty() || sample.timestamp > result.back().timestamp), "Too many or unordered samples");
        for (auto &value : data) if (value.number == 1) {
            require((value.wire == 5 || value.wire == 2) && value.data.size() % 4 == 0, "Invalid sample float");
            for (std::size_t offset = 0; offset < value.data.size(); offset += 4) {
                auto number = std::bit_cast<float>(std::uint32_t(little(value.data.subspan(offset, 4))));
                require(std::isfinite(number) && sample.values.size() < (dimensions ? dimensions : 16), "Invalid sample value or dimension");
                sample.values.push_back(number);
            }
        }
        require(!sample.values.empty() && (!dimensions || sample.values.size() == dimensions), "Incomplete sample vector");
        result.push_back(sample);
    }
    require(!result.empty(), "No standard sensor samples");
    return result;
}
#ifndef UKE_SSC_SAMPLE_TEST
int main(int argc, char **argv) {
    (void)&attributes;
    int fd = -1;
    try {
        require(argc == 2, "Usage: uke-sensor-sample accel|gyro|mag|ambient_light|ambient_light_back|ambient_light_back_strm|cct_front");
        std::string_view type = argv[1];
        const bool xyz = type == "accel" || type == "gyro" || type == "mag";
        const bool cct = type == "cct_front";
        require(xyz || cct || type == "ambient_light" || type == "ambient_light_back" || type == "ambient_light_back_strm", "Unsupported sample type");
        if (cct) std::cerr << "CCT phase=metadata\n";
        fd = socket(AF_QIPCRTR, SOCK_DGRAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
        require(fd >= 0, "QRTR socket unavailable");
        auto found = suids(rpc(fd, request(type), 0, false, cct ? &std::cerr : nullptr), type); close(fd); fd = -1;
        require(found.size() == 1, "Exactly one sensor required for bounded sampling");
        if (cct) std::cerr << "CCT phase=sample request_id=514 expected_event_id=1025\n";
        fd = socket(AF_QIPCRTR, SOCK_DGRAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
        require(fd >= 0, "QRTR sample socket unavailable");
        // The OEM rear stream advertises a minimum of 2 Hz; use its valid rate.
        const Bytes rate = type == "ambient_light_back_strm"
            ? Bytes{0x0d, 0, 0, 0, 0x40} : Bytes{0x0d, 0, 0, 0x80, 0x3f};
        const bool streaming = xyz || type == "ambient_light_back_strm";
        auto event = rpc(fd, client_request(found[0], streaming ? 513 : 514, streaming ? View(rate) : View{}, xyz), 1025, streaming, cct ? &std::cerr : nullptr);
        close(fd); fd = -1; // QRTR disconnect removes this client's sensor requests.
        // shortcut: OEM rear-light channels remain raw until vendor channel units are verified.
        std::cout << std::setprecision(std::numeric_limits<float>::max_digits10);
        for (const auto &sample : samples(event, found[0], xyz ? 3 : 0)) {
            std::cout << type << " timestamp_ticks=" << sample.timestamp << " status=" << sample.status;
            if (cct) std::cout << " units=unverified";
            std::cout << " data=";
            for (float number : sample.values) std::cout << number << ' ';
            std::cout << '\n';
            if (cct) break;
        }
        return 0;
    } catch (const std::exception &e) {
        if (fd >= 0) close(fd);
        std::cerr << e.what() << '\n'; return 1;
    }
}
#endif
