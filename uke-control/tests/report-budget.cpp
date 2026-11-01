// SPDX-License-Identifier: MIT
#include "report-budget.hpp"
#include "report-context.hpp"
#include <cassert>
int main() {
    using namespace uke::reporting;
    assert(select_uid({{1001,true,"wayland"}})==1001);
    assert(select_uid({{1000,true,"wayland"},{1000,true,"x11"}})==1000);
    assert(!select_uid({{1000,true,"wayland"},{1001,true,"x11"}}));
    assert(!select_uid({{0,true,"wayland"},{1001,false,"wayland"},{1002,true,"tty"}}));
    Budget budget("/tmp",4,300,0);assert(budget.allowance(10)==4);budget.charge(4);assert(!budget.allowance(1));
    bool refused=false;try{budget.charge(1);}catch(const std::exception&){refused=true;}assert(refused);
    Budget expired("/tmp",4,0,0);assert(!expired.allowance(1));
    Budget reserved("/tmp",4,300,std::filesystem::space("/tmp").available+1);assert(!reserved.allowance(1));
}
