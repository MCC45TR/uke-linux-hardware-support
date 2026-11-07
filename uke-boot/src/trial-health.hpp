// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>
#include <optional>
#include <stdexcept>

class TrialHealthFailure : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};
inline constexpr int trial_health_rejected=78;

// Times are monotonic boot milliseconds, including suspend. A gap in sampling
// cannot establish continuous health and never contributes to the dwell time.
class TrialHealthWindow {
    int64_t started_,last_;
    std::optional<int64_t> healthy_since_;
public:
    static constexpr int64_t stable_ms=30000,deadline_ms=90000,max_gap_ms=5000;
    explicit TrialHealthWindow(int64_t now):started_(now),last_(now) {
        if(now<0)throw TrialHealthFailure("Invalid trial health clock");
    }
    void check_time(int64_t now) const {
        if(now<last_ || now<started_)throw TrialHealthFailure("Trial health clock moved backwards");
        if(now-started_>=deadline_ms)throw TrialHealthFailure("Trial did not sustain 30 seconds of health within 90 seconds");
    }
    bool observe(int64_t now,bool healthy) {
        check_time(now);
        if(!healthy)healthy_since_.reset();
        else if(!healthy_since_ || now-last_>max_gap_ms)healthy_since_=now;
        last_=now;
        return healthy_since_ && now-*healthy_since_>=stable_ms;
    }
};

template<class Probe,class Clock,class Pause>
static void qualify_trial_health(Probe probe,Clock now,Pause pause) {
    TrialHealthWindow window(now());
    for(unsigned sample=0;sample<91;++sample) {
        window.check_time(now());
        const bool healthy=probe();
        if(window.observe(now(),healthy))return;
        pause();
    }
    throw TrialHealthFailure("Trial health clock failed to establish a bounded observation window");
}
