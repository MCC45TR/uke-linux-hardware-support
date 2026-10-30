// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>
namespace uke::torch {
// A continuous-on ceiling, not an idle timeout. Repeated requests cannot extend it.
class Lease {
    uint64_t deadline_=0;
public:
    static constexpr uint64_t maximum_us=60ULL*1000000;
    void applied(bool on,uint64_t now) {
        if(!on)deadline_=0;
        else if(!deadline_)deadline_=now+maximum_us;
    }
    bool expired(uint64_t now) const {return deadline_ && now>=deadline_;}
    uint64_t remaining(uint64_t now) const {return deadline_>now?deadline_-now:0;}
};
}
