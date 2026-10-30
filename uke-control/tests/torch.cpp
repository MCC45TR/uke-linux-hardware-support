// SPDX-License-Identifier: MIT
#include <cstring>
#include "torch-io.hpp"
#include <functional>
static void refuses(const std::function<void()>& f){bool failed=false;try{f();}catch(...){failed=true;}assert(failed);}
int main() {
    using namespace torch_fixture;
    for(unsigned bad=0;bad<4;++bad){uid=bad==0?0:983;wrong_owner=bad==1;wrong_label=bad==2;wrong_fs=bad==3;refuses([]{uke::torch::Actuator a;});assert(writes.empty());}
    uid=983;wrong_owner=wrong_label=wrong_fs=false;
    {
        uke::torch::Actuator a;assert(brightness==0);
        for(unsigned p=1;p<=100;++p){a.set(p);assert(brightness==(p<100?1:2));assert(a.status().find("\"requested_percent\":"+std::to_string(p)+",")!=std::string::npos);}
        a.enable(false);assert(brightness==0);a.enable(true);assert(brightness==2);
        a.set(17);a.set(0);assert(brightness==0);a.enable(true);assert(brightness==1);
        refuses([&]{a.set(101);});assert(brightness==1);
        fail_level=2;refuses([&]{a.set(100);});assert(brightness==0);fail_level=9;
        bad_readback=true;refuses([&]{a.set(50);});assert(brightness==0);bad_readback=false;
        wrong_provider=true;refuses([&]{a.enable(true);});assert(brightness==0 && writes.back()==0);wrong_provider=false;
        a.enable(true);
    }
    assert(brightness==0 && writes.back()==0);
    const auto n=writes.size();uid=0;assert(uke::torch::prepare_access()==0);assert(uke::torch::prepare_access(true)==0);
    assert(metadata_writes==6 && writes.size()==n);uid=983;refuses([]{uke::torch::prepare_access();});
    std::cout<<"Actual torch actuator: 1–100 mapping, remembered requests, manual off, metadata-only setup, identity/label/ownership refusal and failure/destructor cleanup passed (simulated sysfs)\n";
}
