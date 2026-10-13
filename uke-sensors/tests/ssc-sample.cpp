// SPDX-License-Identifier: MIT
#define UKE_SSC_SAMPLE_TEST
#include "../src/ssc-sample.cpp"
#include <cassert>
template<class F> void rejects(F operation) {
    bool rejected = false;
    try { operation(); } catch (const std::exception &) { rejected = true; }
    assert(rejected);
}
int main() {
    // Independent protobuf vector: sensor {1,2}, event 1025, tick 7, XYZ {1,-2,0}, high accuracy.
    Bytes event{0x0a,18,9,1,0,0,0,0,0,0,0,0x11,2,0,0,0,0,0,0,0,
        0x12,33,0x0d,1,4,0,0,0x11,7,0,0,0,0,0,0,0,0x1a,17,
        0x0d,0,0,0x80,0x3f,0x0d,0,0,0,0xc0,0x0d,0,0,0,0,0x10,3};
    auto values = samples(event, {1,2}, 3);
    assert(values.size() == 1 && values[0].timestamp == 7 && values[0].status == 3);
    assert((values[0].values == std::vector<float>{1,-2,0}));
    assert(samples(event, {1,2}, 0)[0].values == values[0].values);
    rejects([&]{ samples(event, {2,1}, 3); });
    rejects([&]{ samples(event, {1,2}, 1); });
    for (std::size_t n=0; n < event.size(); ++n) rejects([&]{ samples(View(event).first(n), {1,2}, 3); });
    event.back() = 4; rejects([&]{ samples(event, {1,2}, 3); });
    event.back() = 3;
    const Bytes packed{0x0a,18,9,1,0,0,0,0,0,0,0,0x11,2,0,0,0,0,0,0,0,
        0x12,32,0x0d,1,4,0,0,0x11,7,0,0,0,0,0,0,0,0x1a,16,
        0x0a,12,0,0,0x80,0x3f,0,0,0,0xc0,0,0,0,0,0x10,3};
    assert((samples(packed, {1,2}, 3)[0].values == std::vector<float>{1,-2,0}));
    auto bad_packed = packed;
    bad_packed[39] = 11;
    rejects([&]{ samples(bad_packed, {1,2}, 3); });
    event[41] = 0xc0; event[42] = 0x7f; // NaN.
    rejects([&]{ samples(event, {1,2}, 3); });
    assert(datatype("mag") && datatype("ambient_light_back") && datatype("ambient_light_back_strm"));
    rejects([]{ rpc(-1, {}, 0, true); });
    rejects([]{ client_request({1,2}, 1, {}, true); });
    const Bytes batched_wire{0,1,0,0x20,0,51,0,1,48,0,46,0,
        0x0a,18,9,1,0,0,0,0,0,0,0,0x11,2,0,0,0,0,0,0,0,
        0x15,1,2,0,0,0x1a,4,8,1,0x10,1,
        0x22,13,0x0a,4,8,0xa0,0x86,6,0x12,5,0x0d,0,0,0x80,0x3f};
    assert(client_request({1,2}, 513, Bytes{0x0d,0,0,0x80,0x3f}, true) == batched_wire);
    (void)&rpc; (void)&request; (void)&suids; (void)&attributes;
    std::cout << "SSC sample framing, finite floats, dimensions, status and foreign-source checks passed\n";
}
