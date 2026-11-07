// SPDX-License-Identifier: MIT
#include "../src/rtc-clock.hpp"
#include <cassert>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <vector>
static void reject(const std::function<void()> &call){bool refused=false;try{call();}catch(const std::exception&){refused=true;}assert(refused);}
int main(){
    constexpr int64_t initial=1793730000;
    for(const auto zone:{"UTC","Europe/Istanbul","Pacific/Apia"}){
        setenv("TZ",zone,1);tzset();
        for(const int64_t value:{int64_t(1577836800),initial,int64_t(1709164800),int64_t(4102444799)})
            assert(uke_rtc::utc(uke_rtc::calendar(value))==value);
    }
    for(const int64_t value:{int64_t(0),int64_t(1577836799),int64_t(4102444800),INT64_MAX})reject([&]{uke_rtc::calendar(value);});
    for(unsigned fault=0;fault<8;++fault){
        auto value=uke_rtc::calendar(initial);
        switch(fault){case 0:value.tm_year=119;break;case 1:value.tm_mon=12;break;case 2:value.tm_mday=0;break;
        case 3:value.tm_hour=24;break;case 4:value.tm_min=60;break;case 5:value.tm_sec=60;break;
        case 6:value.tm_year=123;value.tm_mon=1;value.tm_mday=29;break;
        case 7:value.tm_mon=3;value.tm_mday=31;break;}
        reject([&]{uke_rtc::utc(value);});
    }
    unsigned reads=0,writes=0,clocks=0;
    std::optional<int64_t> stored;
    auto read=[&]{++reads;return stored;};
    auto write=[&](int64_t utc){++writes;stored=utc;};
    auto clock=[&]{++clocks;return uke_rtc::Sample{initial,true};};
    assert(uke_rtc::synchronize(read,write,[]{return uke_rtc::Sample{initial,false};})==uke_rtc::Result::deferred);
    assert(!reads&&!writes);
    assert(uke_rtc::synchronize(read,write,clock)==uke_rtc::Result::written);
    assert(reads==2&&writes==1&&clocks==3&&stored==initial);
    assert(uke_rtc::synchronize(read,write,clock)==uke_rtc::Result::correct&&writes==1);
    uke_rtc::check(read,[]{return uke_rtc::Sample{initial,false};});
    stored=initial-1;assert(uke_rtc::synchronize(read,write,clock)==uke_rtc::Result::correct&&writes==1);
    stored=initial-60;assert(uke_rtc::synchronize(read,write,clock)==uke_rtc::Result::written&&writes==2);
    // Initial and pre-write snapshots must both authorize synchronized UTC.
    for(unsigned fault=0;fault<4;++fault){
        stored.reset();unsigned count=0,before=writes;
        auto changing=[&]{++count;return uke_rtc::Sample{count==1?initial:initial+(fault==0?-1:fault==1?3:0),count==1||fault<2};};
        if(fault==3)reject([&]{uke_rtc::synchronize([]()->std::optional<int64_t>{throw std::runtime_error("RTC read failed");},write,clock);});
        else reject([&]{uke_rtc::synchronize(read,write,changing);});
        assert(writes==before);
    }
    // Failed writes and mismatched/uninitialized readback cannot report success.
    stored.reset();reject([&]{uke_rtc::synchronize(read,[](int64_t){throw std::runtime_error("write refused");},clock);});
    for(unsigned fault=0;fault<5;++fault){
        stored.reset();unsigned count=0;
        auto after=[&]{++count;return uke_rtc::Sample{count<3?initial:initial+(fault==0?-1:fault==1?4:0),count<3||fault!=2};};
        reject([&]{uke_rtc::synchronize(read,[&](int64_t){stored=fault==3?std::optional<int64_t>{}:std::optional<int64_t>{initial+10};},after);});
    }
    stored=initial+4;reject([&]{uke_rtc::check(read,clock);});
    stored.reset();reject([&]{uke_rtc::check(read,clock);});
    std::cout<<"Secure RTC userspace: UTC/calendar/timezone bounds, NTP-only initialization, idempotence, drift repair, clock/sync changes, read/write failures and readback refusals passed; injected clocks and I/O only\n";
}
