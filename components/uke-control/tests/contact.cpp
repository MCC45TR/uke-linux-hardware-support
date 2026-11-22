// SPDX-License-Identifier: MIT
#include "contact.hpp"
#include <cassert>
int main() {
    uke::input::Contact contact{0,42,0};
    const auto event=[&](unsigned short type,unsigned short code,int value){input_event e{};e.type=type;e.code=code;e.value=value;return contact.observe(e);};
    assert(!event(EV_ABS,ABS_MT_SLOT,1));
    assert(!event(EV_ABS,ABS_MT_TRACKING_ID,43));
    assert(!event(EV_ABS,ABS_MT_TRACKING_ID,-1));
    assert(!event(EV_SYN,SYN_REPORT,0));
    assert(!event(EV_ABS,ABS_MT_SLOT,0));
    assert(!event(EV_ABS,ABS_MT_TRACKING_ID,42));
    assert(event(EV_ABS,ABS_MT_TRACKING_ID,-1)==2);
    assert(event(EV_ABS,ABS_MT_TRACKING_ID,44)==2);
    assert(event(EV_KEY,BTN_TOUCH,0)==2);
    assert(event(EV_SYN,SYN_DROPPED,0)==4);
}
