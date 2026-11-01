// SPDX-License-Identifier: MIT
#pragma once
#include <linux/input.h>
namespace uke::input {
struct Contact {
    int original_slot, original_id, selected_slot;
    // 0: still observed, 2: original contact ended/changed, 4: evidence lost.
    int observe(const input_event& event) {
        if(event.type==EV_SYN&&event.code==SYN_DROPPED)return 4;
        if(event.type==EV_ABS&&event.code==ABS_MT_SLOT)selected_slot=event.value;
        if(event.type==EV_ABS&&event.code==ABS_MT_TRACKING_ID&&
           selected_slot==original_slot&&event.value!=original_id)return 2;
        if(event.type==EV_KEY&&event.code==BTN_TOUCH&&!event.value)return 2;
        return 0;
    }
};
}
