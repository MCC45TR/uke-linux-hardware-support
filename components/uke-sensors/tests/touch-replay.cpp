// SPDX-License-Identifier: GPL-2.0-or-later
// Host-only replay; private raw frames and calibration are never source fixtures.
#include "../src/touch-thp-packet.hpp"
#include "../src/touch-thp-geometry.hpp"
#include <iostream>
int main(int argc,char **argv) {
    try {
        if(argc!=4) throw std::runtime_error("Usage: touch-replay REFERENCE_PACKET OEM_CONFIG CAPTURE");
        nvt::TouchCore core(touch_geometry(argv[2]));
        std::array<std::uint8_t,uke_touch::frame_size> bytes{};
        std::ifstream reference(argv[1],std::ios::binary);
        if(!reference.read(reinterpret_cast<char *>(bytes.data()),bytes.size()) || reference.peek()!=EOF)
            throw std::runtime_error("One reference packet required");
        const auto seed=uke_touch::decode(bytes);
        if(!seed) throw std::runtime_error("Corrupt reference");
        core.process(seed->matrix,seed->counter,seed->scan_mode);
        std::ifstream capture(argv[3],std::ios::binary);
        unsigned count=0,valid=0,contact_frames=0,maximum=0,duplicate_counters=0;
        std::optional<std::uint16_t> previous;
        std::cout<<"frame,counter,scan_mode,contacts,palm\n";
        while(capture.read(reinterpret_cast<char *>(bytes.data()),bytes.size())) {
            if(++count>300) throw std::runtime_error("Capture exceeds 300-frame bound");
            const auto frame=uke_touch::decode(bytes);
            if(!frame) continue;
            valid++;
            duplicate_counters+=previous && *previous==frame->counter;
            previous=frame->counter;
            const auto result=core.process(frame->matrix,frame->counter,frame->scan_mode);
            contact_frames+=!result.slots.empty();
            maximum=std::max(maximum,unsigned(result.slots.size()));
            std::cout<<count<<','<<frame->counter<<','<<unsigned(frame->scan_mode)<<','<<result.slots.size()<<','<<result.palm_active<<'\n';
        }
        if(!capture.eof() || capture.gcount()!=0 || !valid) throw std::runtime_error("Empty, invalid or truncated capture");
        std::cerr<<"frames="<<count<<" valid="<<valid<<" contact_frames="<<contact_frames<<" maximum_slots="<<maximum
                 <<" duplicate_counters="<<duplicate_counters<<" (offline replay; physical finger count unknown)\n";
        return 0;
    } catch(const std::exception &e) { std::cerr<<e.what()<<'\n'; return 1; }
}
