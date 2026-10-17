// SPDX-License-Identifier: MIT
#define UKE_SSC_TEST
#include "../src/ssc-discovery.cpp"
#include <cassert>

template<class F> void rejects(F operation) {
    bool rejected = false;
    try { operation(); } catch (const std::exception &) { rejected = true; }
    assert(rejected);
}
int main() {
    // Independent wire vector from pinned Qualcomm QMI/protobuf field definitions.
    const Bytes accel{
        0,1,0,0x20,0,0x33,0, 1,0x30,0,0x2e,0,
        0x0a,0x12,0x09,0xab,0xab,0xab,0xab,0xab,0xab,0xab,0xab,
        0x11,0xab,0xab,0xab,0xab,0xab,0xab,0xab,0xab,
        0x15,0,2,0,0, 0x1a,4,8,1,0x10,1,
        0x22,0x0d,0x12,0x0b,0x0a,5,'a','c','c','e','l',0x10,0,0x18,0
    };
    assert(request("accel") == accel);
    assert(tlvs(accel).size() == 1);
    auto all = request("");
    assert(all.size() == accel.size()-5 && all[5] == 0x2e && all[8] == 0x2b && all[10] == 0x29);
    rejects([]{ request("factory_test"); });
    rejects([]{ fields(Bytes{0x0a, 5, 1}); });
    rejects([]{ fields(Bytes{0}); });
    rejects([]{ fields(Bytes{0x0b}); });
    rejects([]{ fields(Bytes{8,255,255,255,255,255,255,255,255,255,2}); });
    rejects([]{ one(fields(Bytes{8,1,8,2}),1,0); });
    for (std::size_t i = 0; i < accel.size(); ++i) rejects([&]{ tlvs(View(accel).first(i)); });
    Bytes event{0x0a,0x12,9,0xab,0xab,0xab,0xab,0xab,0xab,0xab,0xab,
        0x11,0xab,0xab,0xab,0xab,0xab,0xab,0xab,0xab,
        0x12,0x19,0x0d,0,3,0,0,0x11,1,0,0,0,0,0,0,0,
        0x1a,9,0x0a,5,'a','c','c','e','l',0x10,0};
    // Last field is an intentionally malformed discovered SUID, not a false empty result.
    rejects([&]{ suids(event,"accel"); });
    event.erase(event.end()-2,event.end());
    event[21]=0x17; event[37]=7;
    assert(suids(event,"accel").empty());
    event.insert(event.end(), {0x12,18,9,1,0,0,0,0,0,0,0,0x11,2,0,0,0,0,0,0,0});
    event[21]=0x2b; event[37]=0x1b;
    assert((suids(event,"accel") == std::vector<Uid>{{1,2}}));
    for (std::size_t i = 20; i < event.size(); ++i) rejects([&]{ suids(View(event).first(i),"accel"); });
    rejects([&]{ suids(event,"gyro"); });
    event[3]=0; rejects([&]{ suids(event,"accel"); });
    // Keep network symbols compiled even in a host-only test.
    static_assert(sizeof(sockaddr_qrtr) >= 10);
    (void)&rpc;
    const Bytes metadata{
        0x0a,18,9,1,0,0,0,0,0,0,0,0x11,2,0,0,0,0,0,0,0,
        0x12,33,0x0d,128,0,0,0,0x11,1,0,0,0,0,0,0,0,0x1a,17,
        0x0a,15,8,2,0x12,11,0x0a,9,0x12,7,'v','i','r','t','u','a','l'
    };
    assert(attributes(metadata,{1,2}) == "name=not returned; type=virtual; available=not returned; physical=not returned");
    assert(attributes(metadata,{1,2},true).ends_with("\nattribute_id=2 protobuf_hex=0a0912077669727475616c"));
    rejects([&]{attributes(metadata,{2,1});});
    rejects([]{request("",{1,2},3);});
    rejects([]{request("accel",{1,2},1);});
    auto attr_request=request("",{1,2},1);
    const Bytes attr_wire{
        0,1,0,0x20,0,40,0,1,37,0,35,0,
        0x0a,18,9,1,0,0,0,0,0,0,0,0x11,2,0,0,0,0,0,0,0,
        0x15,1,0,0,0,0x1a,4,8,1,0x10,1,0x22,2,0x12,0
    };
    assert(attr_request==attr_wire);
    std::cout << "SSC one-shot request, framing, truncation, overflow and foreign-event checks passed\n";
}
