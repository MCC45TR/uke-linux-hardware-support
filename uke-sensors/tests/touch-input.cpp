// SPDX-License-Identifier: GPL-3.0-or-later
// Exercise the actual publisher through a pipe; no uinput or SPI access.
#define main touch_lab_main
#include "../src/touch-thp.cpp"
#undef main
#include <cassert>
int main() {
    int pipefd[2]; assert(pipe2(pipefd,O_NONBLOCK|O_CLOEXEC)==0);
    std::array<gpio_v2_line_event,16> edges{};
    assert(::write(pipefd[1],edges.data(),sizeof(edges))==sizeof(edges));
    assert(consume_touch_irq(pipefd[0])==edges.size());
    gpio_v2_line_event edge{};
    assert(::read(pipefd[0],&edge,sizeof(edge))==-1 && errno==EAGAIN);
    assert(::write(pipefd[1],&edge,1)==1);
    bool partial_rejected=false;
    try{consume_touch_irq(pipefd[0]);}catch(const std::exception&){partial_rejected=true;}
    assert(partial_rejected);
    bool empty_rejected=false;
    try{consume_touch_irq(pipefd[0]);}catch(const std::exception&){empty_rejected=true;}
    assert(empty_rejected);
    std::array<int,nvt::kFingerSlots> ids; ids.fill(-1);
    nvt::Geometry geometry; geometry.width=32000; geometry.height=21360;
    std::vector<nvt::Slot> slots;
    for(int i=0;i<10;i++) {
        nvt::Slot slot; slot.number=i; slot.tracking_id=100+i;
        slot.contact.x=1000+i*2000; slot.contact.y=2000+i*1000;
        slots.push_back(slot);
    }
    const auto publish=[&](const std::vector<nvt::Slot> &contacts) {
        input_report(pipefd[1],contacts,ids,geometry);
        std::array<input_event,64> bytes{};
        const auto size=::read(pipefd[0],bytes.data(),sizeof(bytes));
        assert(size>0 && size%sizeof(input_event)==0);
        std::vector<input_event> result(bytes.begin(),bytes.begin()+size/sizeof(input_event));
        assert(result.front().type==EV_KEY && result.front().code==BTN_TOUCH && result.front().value==!contacts.empty());
        assert(result.back().type==EV_SYN && result.back().code==SYN_REPORT);
        return result;
    };
    const auto first=publish(slots);
    assert(std::count_if(first.begin(),first.end(),[](auto e){return e.type==EV_ABS && e.code==ABS_MT_TRACKING_ID;})==10);
    const auto y=std::find_if(first.begin(),first.end(),[](auto e){return e.type==EV_ABS && e.code==ABS_MT_POSITION_Y;});
    assert(y!=first.end() && y->value==geometry.height-1-2000);
    const auto moved=publish(slots);
    assert(std::none_of(moved.begin(),moved.end(),[](auto e){return e.type==EV_ABS && e.code==ABS_MT_TRACKING_ID;}));
    slots.erase(slots.begin()+4);
    const auto lifted=publish(slots);
    assert(std::count_if(lifted.begin(),lifted.end(),[](auto e){return e.type==EV_ABS && e.code==ABS_MT_TRACKING_ID && e.value==-1;})==1);
    assert(ids[4]==-1 && ids[3]==103 && ids[5]==105);
    slots[0].tracking_id=500;
    const auto replaced=publish(slots);
    assert(std::count_if(replaced.begin(),replaced.end(),[](auto e){return e.type==EV_ABS && e.code==ABS_MT_TRACKING_ID && e.value==500;})==1);
    const auto released=publish({});
    assert(std::count_if(released.begin(),released.end(),[](auto e){return e.type==EV_ABS && e.code==ABS_MT_TRACKING_ID && e.value==-1;})==9);
    assert(std::all_of(ids.begin(),ids.end(),[](int id){return id==-1;}));
    for(unsigned trial=0;trial<4;trial++) {
        auto invalid=slots;
        if(trial==0) invalid[1].number=invalid[0].number;
        if(trial==1) invalid[1].tracking_id=invalid[0].tracking_id;
        if(trial==2) invalid[0].contact.x=geometry.width;
        if(trial==3) invalid[0].number=nvt::kFingerSlots;
        bool rejected=false;
        try{input_report(pipefd[1],invalid,ids,geometry);}catch(const std::exception&){rejected=true;}
        assert(rejected);
        input_event byte{}; assert(::read(pipefd[0],&byte,sizeof(byte))==-1 && errno==EAGAIN);
        assert(std::all_of(ids.begin(),ids.end(),[](int id){return id==-1;}));
    }
    close(pipefd[0]); close(pipefd[1]);
    std::cout<<"Actual type-B publisher and IRQ batch: ten contacts, stable IDs, independent lift, replacement, release and invalid input passed (host only)\n";
}
