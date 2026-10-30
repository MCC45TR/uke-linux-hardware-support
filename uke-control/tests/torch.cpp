// SPDX-License-Identifier: MIT
#include "torch-io.hpp"
#include <functional>
static void refuses(const std::function<void()>& f){bool failed=false;try{f();}catch(...){failed=true;}assert(failed);}
int main() {
    using namespace torch_fixture;
    for(unsigned bad=0;bad<4;++bad){uid=bad==0?0:983;wrong_owner=bad==1;wrong_label=bad==2;wrong_type=bad==3;refuses([]{uke::torch::Actuator a;});assert(requests.empty());}
    uid=983;wrong_owner=wrong_label=wrong_type=false;
    {
        uke::torch::Actuator a;
        for(unsigned p=0;p<=100;++p){a.set(p);assert(requests.back()==std::make_pair(static_cast<unsigned long>(UKE_TORCH_SET),p));}
        const auto n=requests.size();refuses([&]{a.set(101);});assert(requests.size()==n);
        a.enable(false);assert(requests.back()==std::make_pair(static_cast<unsigned long>(UKE_TORCH_ENABLE),0U));
        a.enable(true);assert(requests.back()==std::make_pair(static_cast<unsigned long>(UKE_TORCH_ENABLE),1U));
        fail_request=true;refuses([&]{a.set(50);});assert(requests.size()==n+3);fail_request=false;
        assert(a.status().find("\"policy_owner\":\"kernel\"")!=std::string::npos);
        assert(a.status().find("\"request_step_percent\":1")!=std::string::npos);
        assert(a.status().find("\"brightness_rounding\":\"up_to_provider_step\"")!=std::string::npos);
        response.requested_percent=17;response.brightness=1;
        const auto s=a.status();assert(s.find("\"requested_percent\":17")!=std::string::npos && s.find("\"brightness_percent\":50")!=std::string::npos);
        response.brightness_valid=0;response.last_io_error=-EIO;
        const auto unknown=a.status();assert(unknown.find("\"on\":null")!=std::string::npos && unknown.find("\"brightness_percent\":null")!=std::string::npos);
        const auto valid=response;
        for(unsigned field=0;field<7;++field){
            response=valid;
            if(field==0)response.abi=99;
            if(field==1)response.brightness=3;
            if(field==2)response.max_microamp=20001;
            if(field==3)response.requested_percent=0;
            if(field==4)response.suspended=2;
            if(field==5)response.last_io_error=-4096;
            if(field==6)response.reserved=1;
            refuses([&]{a.status();});
        }
        response=valid;fail_status=true;refuses([&]{a.status();});fail_status=false;
    }
    assert(closed==4);
    const auto n=requests.size();response.abi=99;refuses([]{uke::torch::Actuator a;});assert(closed==5 && requests.size()==n);
    std::cout<<"Actual torch client: unchanged request transport, kernel status/error validation, device ownership/type/label refusal and descriptor cleanup passed (simulated ioctl)\n";
}
