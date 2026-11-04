// SPDX-License-Identifier: MIT
#include "../src/trial-health.hpp"
#include <cassert>
#include <functional>
#include <iostream>

static void reject(const std::function<void()> &call) {
    bool refused=false;
    try {call();}catch(const TrialHealthFailure &){refused=true;}
    assert(refused);
}
int main() {
    static_assert(trial_health_rejected==78);
    int64_t now=1000000;
    unsigned samples=0,pauses=0;
    qualify_trial_health([&]{++samples;return true;},[&]{return now;},[&]{now+=1000;++pauses;});
    assert(samples==31 && pauses==30 && now==1030000);
    now=0;samples=0;pauses=0;
    qualify_trial_health([&]{++samples;return now!=20000;},[&]{return now;},[&]{now+=1000;++pauses;});
    assert(now==51000 && samples==52 && pauses==51);
    for(unsigned pattern=0;pattern<3;++pattern) {
        now=0;samples=0;
        reject([&]{qualify_trial_health([&]{++samples;return pattern==0?false:pattern==1?now%2000==0:now<29000;},
            [&]{return now;},[&]{now+=1000;});});
        assert(now==90000 && samples==90);
    }
    // Delayed probes, scheduling gaps and suspend never count as observed health.
    TrialHealthWindow delayed(0);
    assert(!delayed.observe(0,true));
    assert(!delayed.observe(29000,true));
    for(now=30000;now<59000;now+=1000)assert(!delayed.observe(now,true));
    assert(delayed.observe(59000,true));
    TrialHealthWindow gap(0);
    for(now=0;now<=25000;now+=1000)assert(!gap.observe(now,true));
    assert(!gap.observe(31000,true));
    for(now=32000;now<61000;now+=1000)assert(!gap.observe(now,true));
    assert(gap.observe(61000,true));
    TrialHealthWindow suspended(0);assert(!suspended.observe(0,true));
    reject([&]{suspended.observe(90000,true);});
    TrialHealthWindow backwards(10);assert(!backwards.observe(10,true));
    reject([&]{backwards.observe(9,true);});
    reject([]{TrialHealthWindow bad(-1);});
    now=0;
    reject([&]{qualify_trial_health([&]{now+=90000;return true;},[&]{return now;},[]{});});
    samples=0;
    reject([&]{qualify_trial_health([&]{++samples;return true;},[]{return int64_t(0);},[]{});});
    assert(samples==91);
    std::cout<<"Trial acceptance: continuous 30-second health, transient reset, flapping/deadline refusal, delayed probes, suspend, frozen and reversed clocks passed; injected clocks, no device I/O\n";
}
