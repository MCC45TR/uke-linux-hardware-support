// SPDX-License-Identifier: MIT
#define UKE_SSC_SAMPLE_TEST
#include "../shared/ssc-sample.cpp"
#include <cassert>
template<class F> void rejects(F operation) {
    bool rejected = false;
    try { operation(); } catch (const std::exception &) { rejected = true; }
    assert(rejected);
}
static Bytes capture_attributes(bool available=true, unsigned stream=0, std::string_view api="sns_sensor_temperature.proto") {
    const auto text=[](std::string_view s){Bytes b;length_field(b,2,View(reinterpret_cast<const std::uint8_t*>(s.data()),s.size()));return b;};
    Bytes streaming{0x21};put(streaming,stream,8);
    const std::array<Bytes,4> values{text("sensor_temperature"),Bytes{0x28,std::uint8_t(available)},text(api),streaming};
    const std::array<unsigned,4> ids{2,3,5,16};Bytes payload;
    for(unsigned i=0;i<4;++i){Bytes value;length_field(value,1,values[i]);Bytes attr{8,std::uint8_t(ids[i])};length_field(attr,2,value);length_field(payload,1,attr);}
    Bytes uid{9};put(uid,1,8);uid.push_back(0x11);put(uid,2,8);
    Bytes e{0x0d};put(e,128,4);e.push_back(0x11);put(e,1,8);length_field(e,3,payload);
    Bytes root;length_field(root,1,uid);length_field(root,2,e);return root;
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
    // An independently reconstructed descriptor-882 event is not the active HAL's 1025 format.
    const Bytes cct{0x0a,18,9,1,0,0,0,0,0,0,0,0x11,2,0,0,0,0,0,0,0,
        0x12,61,0x0d,0x72,3,0,0,0x11,7,0,0,0,0,0,0,0,0x1a,45,
        0x0d,0,0,0xf7,0x42,0x10,1,0x18,2,0x20,3,0x28,4,0x30,5,
        0x3d,0,0,0x80,0x3e,0x45,0,0,0,0x3f,0x4d,0,0,0,0x3f,
        0x55,0,0,0x34,0x42,0x5d,0,0,0x80,0x40,0x65,0,0,0x48,0x42};
    rejects([&]{ samples(cct, {1,2}, 0); });
    auto bad_cct = cct;
    bad_cct[23] = 0x73; // Event 883 has a different descriptor.
    rejects([&]{ samples(bad_cct, {1,2}, 0); });
    auto bad_light = light; bad_light[28] = 0;
    rejects([&]{ samples(bad_light, {1,2}, 0); });
    bad_light = light;
    bad_light.insert(bad_light.end(), light.begin() + 20, light.end());
    rejects([&]{ samples(bad_light, {1,2}, 0); });
    Bytes many(light.begin(), light.begin() + 20);
    for (unsigned n = 1; n <= 33; ++n) {
        Bytes next(light.begin() + 20, light.end()); next[8] = n;
        many.insert(many.end(), next.begin(), next.end());
    }
    rejects([&]{ samples(many, {1,2}, 0); });
    const Bytes cct_request{0,1,0,0x20,0,40,0,1,37,0,35,0,
        0x0a,18,
        0x09,1,0,0,0,0,0,0,0,
        0x11,2,0,0,0,0,0,0,0,
        0x15,2,2,0,0,0x1a,4,8,1,0x10,1,0x22,2,0x12,0};
    assert(cct_request.size() == 47);
    assert(client_request({1,2}, 514, {}) == cct_request);
    // Uke's flicker HAL inherits the standard 1025 float event and on-change request.
    Bytes flicker=light;
    assert(samples(flicker,{1,2},0)[0].values.size()==2);
    flicker.back()=0;
    assert(samples(flicker,{1,2},0)[0].status==0); // Preserve unreliable/occluded observations.
    flicker[24]=3; // The unrelated sns_flicker.proto descriptor uses event 769.
    rejects([&]{ samples(flicker,{1,2},0); });
    assert(datatype("flicker"));
    const Bytes seven_channel_flicker{0x0a,18,9,1,0,0,0,0,0,0,0,0x11,2,0,0,0,0,0,0,0,
        0x12,48,0x0d,1,4,0,0,0x11,7,0,0,0,0,0,0,0,0x1a,32,0x0a,28,
        0,0,0xc8,0x42,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0x10,3};
    assert(samples(seven_channel_flicker,{1,2},0)[0].values.size()==7);
    assert(samples(seven_channel_flicker,{1,2},0)[0].values[0]==100);
    std::ostringstream trace;
    unsigned traced = 0;
    trace_rpc_event(nullptr, traced, 1025);
    assert(traced == 0);
    for (unsigned id = 1000; id < 1010; ++id) trace_rpc_event(&trace, traced, id);
    const auto logged = trace.str();
    assert(traced == 8 && std::count(logged.begin(), logged.end(), '\n') == 8);
    assert(logged.find("event_id=1000") != std::string::npos && logged.find("event_id=1008") == std::string::npos);
    assert(duration("1")==1 && duration("60")==60);
    for (auto invalid:{"0","61","-1","1x","1.0",""}) rejects([&]{duration(invalid);});
    Capture capture;
    capture.accept({100,3,{1,2,3}},1'000'000'000);
    capture.accept({120,3,{3,4,5}},2'000'000'000);
    assert(capture.count==2 && capture.mean[0]==2 && capture.m2[0]==2);
    assert(capture.min_dt==20 && capture.max_dt==20 && capture.integral[0]==40);
    rejects([&]{capture.accept({120,3,{3,4,5}},3'000'000'000);});
    rejects([&]{capture.accept({130,3,{3}},3'000'000'000);});
    rejects([&]{capture_metadata({}, {1,2}, "sensor_temperature");});
    capture_metadata(capture_attributes(),{1,2},"sensor_temperature");
    rejects([]{capture_metadata(capture_attributes(false),{1,2},"sensor_temperature");});
    rejects([]{capture_metadata(capture_attributes(true,1),{1,2},"sensor_temperature");});
    rejects([]{capture_metadata(capture_attributes(true,0,"sns_ambient_light.proto"),{1,2},"sensor_temperature");});
    rejects([]{capture_metadata(capture_attributes(),{2,1},"sensor_temperature");});
    (void)&rpc; (void)&request; (void)&suids; (void)&attributes; (void)&receipt_time;
    std::cout << "SSC sample framing, finite floats, dimensions, status and foreign-source checks passed\n";
}
