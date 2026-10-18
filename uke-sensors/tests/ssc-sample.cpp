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
    // Front ALS may include lux and raw channels; preserve the complete vector.
    const Bytes light{0x0a,18,9,1,0,0,0,0,0,0,0,0x11,2,0,0,0,0,0,0,0,
        0x12,28,0x0d,1,4,0,0,0x11,7,0,0,0,0,0,0,0,0x1a,12,
        0x0a,8,0,0,0,0x42,0,0,0x80,0x43,0x10,3};
    assert((samples(light, {1,2}, 0)[0].values == std::vector<float>{32,256}));
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
    // Independent Uke descriptor vector: event 882, seven float and five int32 fields.
    const Bytes cct{0x0a,18,9,1,0,0,0,0,0,0,0,0x11,2,0,0,0,0,0,0,0,
        0x12,61,0x0d,0x72,3,0,0,0x11,7,0,0,0,0,0,0,0,0x1a,45,
        0x0d,0,0,0xf7,0x42,0x10,1,0x18,2,0x20,3,0x28,4,0x30,5,
        0x3d,0,0,0x80,0x3e,0x45,0,0,0,0x3f,0x4d,0,0,0,0x3f,
        0x55,0,0,0x34,0x42,0x5d,0,0,0x80,0x40,0x65,0,0,0x48,0x42};
    const auto color = cct_samples(cct, {1,2});
    assert(color.size() == 1 && color[0].timestamp == 7);
    assert((color[0].values == std::array<double,12>{123.5,1,2,3,4,5,0.25,0.5,0.5,45,4,50}));
    rejects([&]{ cct_samples(cct, {2,1}); });
    rejects([&]{ samples(cct, {1,2}, 0); });
    rejects([&]{ cct_samples(packed, {1,2}); });
    for (std::size_t n = 0; n < cct.size(); ++n) rejects([&]{ cct_samples(View(cct).first(n), {1,2}); });
    auto bad_cct = cct;
    bad_cct[23] = 0x73; // Event 883 has a different descriptor.
    rejects([&]{ cct_samples(bad_cct, {1,2}); });
    bad_cct = cct; bad_cct[28] = 0;
    rejects([&]{ cct_samples(bad_cct, {1,2}); });
    bad_cct = cct; bad_cct[41] = 0xc0; bad_cct[42] = 0x7f;
    rejects([&]{ cct_samples(bad_cct, {1,2}); });
    bad_cct = cct;
    bad_cct.erase(bad_cct.begin() + 44);
    bad_cct.insert(bad_cct.begin() + 44, {0x80,0x80,0x80,0x80,8}); // Positive 2^31 is not int32.
    bad_cct[21] += 4; bad_cct[37] += 4;
    rejects([&]{ cct_samples(bad_cct, {1,2}); });
    bad_cct = cct;
    bad_cct.erase(bad_cct.begin() + 43, bad_cct.begin() + 45);
    bad_cct[21] -= 2; bad_cct[37] -= 2; // Complete framing with a missing required field.
    rejects([&]{ cct_samples(bad_cct, {1,2}); });
    bad_cct = cct; bad_cct[43] = 0x18; // Duplicate green_raw and missing red_raw.
    rejects([&]{ cct_samples(bad_cct, {1,2}); });
    bad_cct = cct;
    bad_cct.erase(bad_cct.begin() + 44);
    bad_cct.insert(bad_cct.begin() + 44, {0x80,0x80,0x80,0x80,0xf8,0xff,0xff,0xff,0xff,1});
    bad_cct[21] += 9; bad_cct[37] += 9;
    assert(cct_samples(bad_cct, {1,2})[0].values[1] == INT32_MIN);
    bad_cct = cct;
    bad_cct.insert(bad_cct.end(), cct.begin() + 20, cct.end());
    rejects([&]{ cct_samples(bad_cct, {1,2}); }); // Duplicate timestamp.
    Bytes many(cct.begin(), cct.begin() + 20);
    for (unsigned n = 1; n <= 33; ++n) {
        Bytes next(cct.begin() + 20, cct.end()); next[8] = n;
        many.insert(many.end(), next.begin(), next.end());
    }
    rejects([&]{ cct_samples(many, {1,2}); });
    const Bytes cct_request{0,1,0,0x20,0,40,0,1,37,0,35,0,
        0x0a,18,9,1,0,0,0,0,0,0,0x11,2,0,0,0,0,0,0,0,
        0x15,2,2,0,0,0x1a,4,8,1,0x10,1,0x22,2,0x12,0};
    assert(client_request({1,2}, 514, {}) == cct_request);
    (void)&rpc; (void)&request; (void)&suids; (void)&attributes;
    std::cout << "SSC sample framing, finite floats, dimensions, status and foreign-source checks passed\n";
}
