// SPDX-License-Identifier: GPL-2.0-or-later
#include "../src/touch-thp-packet.hpp"
#include "../src/touch-thp-geometry.hpp"
#include <cassert>
#include <iostream>
#include <vector>
int main(int argc,char **argv) {
    assert(argc==3);
    const auto geometry=touch_geometry(argv[2]);
    assert(geometry.width==32000 && geometry.height==21360);
    assert(uke_touch::display_y(0,geometry.height)==geometry.height-1);
    assert(uke_touch::display_y(geometry.height-1,geometry.height)==0);
    for(const auto &[coordinate,height]:std::array<std::pair<int,int>,3>{{
        {-1,geometry.height},{geometry.height,geometry.height},{0,0}}}) {
        bool invalid=false;
        try{uke_touch::display_y(coordinate,height);}catch(const std::exception&){invalid=true;}
        assert(invalid);
    }
    std::ifstream input(argv[1],std::ios::binary);
    std::array<std::uint8_t,uke_touch::frame_size> bytes{};
    assert(bool(input.read(reinterpret_cast<char *>(bytes.data()),bytes.size())));
    const auto frame=uke_touch::decode(bytes); assert(frame);
    auto bad=bytes; bad[320]^=1; assert(!uke_touch::decode(bad));
    bad=bytes; bad[256+48]=59; assert(!uke_touch::decode(bad));
    bad=bytes; bad[256+8]=0xff; assert(!uke_touch::decode(bad));
    bad=bytes; bad[256+16]^=1; assert(!uke_touch::decode(bad));
    bool rejected=false;
    try{uke_touch::decode(std::span(bytes).first(100));}catch(const std::exception&){rejected=true;}
    assert(rejected);
    nvt::TouchCore core(geometry);
    nvt::Matrix baseline{}; baseline.fill(10000);
    core.process(baseline,0,2);
    auto touch=baseline;
    for(int r=10;r<13;r++) for(int c=15;c<18;c++) touch[r*60+c]+=800;
    for(int r=26;r<29;r++) for(int c=40;c<43;c++) touch[r*60+c]+=900;
    touch[11*60+16]+=400; touch[27*60+41]+=400;
    nvt::FrameResult result;
    for(unsigned count=1;count<=4;count++) {
        result=core.process(touch,count,4);
        std::cout<<"Synthetic frame "<<count<<": contacts="<<result.contacts.size()<<" slots="<<result.slots.size()<<std::endl;
    }
    assert(result.slots.size()==2);
    assert(result.slots[0].tracking_id!=result.slots[1].tracking_id);
    for(const auto &slot:result.slots) {
        assert(slot.contact.x>=0 && slot.contact.x<geometry.width);
        assert(slot.contact.y>=0 && slot.contact.y<geometry.height);
    }
    result=core.process(baseline,5,4); assert(result.slots.empty());
    core.reset(); core.process(baseline,0,2);
    const auto four_fingers=[&](int shift,bool lifted) {
        auto matrix=baseline;
        const std::array<std::pair<int,int>,4> centres{{{9,12},{9,44+shift},{29,12},{29,44}}};
        for(unsigned finger=0;finger<centres.size();finger++) {
            if(lifted && finger==1) continue;
            const auto [row,column]=centres[finger];
            for(int r=row-1;r<=row+1;r++) for(int c=column-1;c<=column+1;c++) matrix[r*60+c]+=800;
            matrix[row*60+column]+=400;
        }
        return matrix;
    };
    for(unsigned count=1;count<=4;count++) result=core.process(four_fingers(0,false),count,4);
    assert(result.slots.size()==4);
    std::array<int,4> ids{};
    for(unsigned slot=0;slot<4;slot++) ids[slot]=result.slots[slot].tracking_id;
    for(unsigned count=5;count<=8;count++) {
        result=core.process(four_fingers(count-4,false),count,4);
        assert(result.slots.size()==4);
        for(unsigned slot=0;slot<4;slot++) assert(result.slots[slot].tracking_id==ids[slot]);
    }
    result=core.process(four_fingers(4,true),9,4); assert(result.slots.size()==3);
    for(const auto &slot:result.slots) assert(std::find(ids.begin(),ids.end(),slot.tracking_id)!=ids.end());
    result=core.process(baseline,10,4); assert(result.slots.empty());
    core.reset(); core.process(baseline,0,2);
    auto ten=baseline;
    for(int row:{9,29}) for(int column:{7,18,29,40,51}) {
        for(int r=row-1;r<=row+1;r++) for(int c=column-1;c<=column+1;c++) ten[r*60+c]+=800;
        ten[row*60+column]+=400;
    }
    for(unsigned count=1;count<=4;count++) result=core.process(ten,count,4);
    assert(result.slots.size()==10);
    for(unsigned first=0;first<result.slots.size();first++) for(unsigned second=first+1;second<result.slots.size();second++)
        assert(result.slots[first].tracking_id!=result.slots[second].tracking_id);
    result=core.process(baseline,5,4); assert(result.slots.empty());
    std::cout<<"Packet integrity, bounded OEM geometry, two/four/ten slots, independent motion and release passed (host only)\n";
}
