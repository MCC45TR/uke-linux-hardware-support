// SPDX-License-Identifier: MIT
#include "counters.hpp"
#include <cassert>
#include <cmath>
#include <cstdio>
int main(){
    using uke::unsigned_value;
    assert(unsigned_value(" 123\n")==123);
    assert(unsigned_value("18446744073709551615")==UINT64_MAX);
    for(auto value:{""," \n","-1","+1","12 ns","18446744073709551616","1 2"})assert(!unsigned_value(value));
    uke::Activity rate;
    assert(!rate.sample(100,1000));
    assert(rate.sample(150,1100)==50.0);
    assert(rate.sample(150,1200)==0.0);
    assert(rate.sample(400,1300)==100.0);
    assert(!rate.sample(1,1400));
    assert(rate.sample(11,1500)==10.0);
    assert(!rate.sample(12,1500));
    rate.reset();assert(!rate.sample(100,2000));
    assert(std::abs(*rate.sample(101,3000)-0.1)<0.000001);
    std::puts("Adreno counters: strict parsing, baseline, idle, reset, zero time and saturation passed");
}
