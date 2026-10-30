// SPDX-License-Identifier: MIT
// Actual broker handlers/authorization on a private bus; only ioctl is simulated.
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <sys/ioctl.h>
#include <thread>
#include <systemd/sd-bus.h>
#include "../shared/uke-rgb.h"
#include "torch-io.hpp"
static uke_rgb_status fake_status{UKE_RGB_ABI,UKE_RGB_OFF,0,0,UKE_RGB_LIMIT,0,0,82,1,0,0,0,0,0};
static unsigned changes;
static int test_ioctl(int fd,unsigned long command,void* arg) {
    assert(fd==91);
    if(command==UKE_RGB_STATUS){*static_cast<uke_rgb_status*>(arg)=fake_status;return 0;}
    ++changes;
    if(command==UKE_RGB_SET) {
        auto c=*static_cast<uke_rgb_color*>(arg);assert(c.abi==1 && c.rgb<=0xffffff && c.brightness_percent<=100 && !c.reserved);
        fake_status.mode=UKE_RGB_MANUAL;fake_status.rgb=c.rgb;fake_status.requested_rgb=c.rgb;
        fake_status.brightness=c.brightness_percent*8/100;
    } else if(command==UKE_RGB_TEST_COLORS) {
        if(fake_status.mode==UKE_RGB_TEST){errno=EBUSY;return -1;}
        fake_status.mode=UKE_RGB_TEST;fake_status.test_remaining_ms=10000;
    } else {assert(command==UKE_RGB_AUTOMATIC);fake_status.mode=UKE_RGB_OFF;fake_status.test_remaining_ms=0;}
    return 0;
}
#define ioctl test_ioctl
#define main rgb_broker_main
#include "../src/rgb-service.cpp"
#undef main
#undef ioctl
static std::atomic<bool> running{true},allowed{true},ready_auth{false},ready_service{false};
static int authorize_mock(sd_bus_message* m,void*,sd_bus_error*) {
    assert(sd_bus_message_enter_container(m,'r',"sa{sv}")>0);
    const char* subject=nullptr;assert(sd_bus_message_read(m,"s",&subject)>0 && std::string(subject)=="system-bus-name");
    assert(sd_bus_message_skip(m,"a{sv}")>0);assert(sd_bus_message_exit_container(m)>0);
    const char* action=nullptr;assert(sd_bus_message_read(m,"s",&action)>0 &&
        (std::string(action)=="org.senemos.uke.rgb.control" || std::string(action)==uke::torch::action));
    assert(sd_bus_message_skip(m,"a{ss}")>0);
    unsigned flags=99;const char* cancel=nullptr;
    assert(sd_bus_message_read(m,"us",&flags,&cancel)>0 && flags==0 && std::string(cancel).empty());
    return sd_bus_reply_method_return(m,"(bba{ss})",allowed.load()?1:0,0,0);
}
static const sd_bus_vtable auth_methods[]={SD_BUS_VTABLE_START(0),
    SD_BUS_METHOD("CheckAuthorization","(sa{sv})sa{ss}us","(bba{ss})",authorize_mock,SD_BUS_VTABLE_UNPRIVILEGED),SD_BUS_VTABLE_END};
static void server(bool auth) {
    sd_bus* bus=nullptr;assert(sd_bus_open_user(&bus)>=0);State state,torch;state.fd=91;
    if(auth) {
        assert(sd_bus_add_object_vtable(bus,nullptr,"/org/freedesktop/PolicyKit1/Authority","org.freedesktop.PolicyKit1.Authority",auth_methods,nullptr)>=0);
        assert(sd_bus_request_name(bus,"org.freedesktop.PolicyKit1",0)>=0);ready_auth=true;
    } else {
        torch.torch=std::make_unique<uke::torch::Actuator>();
        assert(sd_bus_add_object_vtable(bus,nullptr,uke::rgb::object_path,uke::rgb::interface,methods,&state)>=0);
        assert(sd_bus_add_object_vtable(bus,nullptr,uke::torch::object_path,uke::torch::interface,torch_methods,&torch)>=0);
        assert(sd_bus_request_name(bus,uke::torch::bus_name,0)>=0);
        assert(sd_bus_request_name(bus,uke::rgb::bus_name,0)>=0);ready_service=true;
    }
    while(running) {int r;do{r=sd_bus_process(bus,nullptr);assert(r>=0);}while(r>0);assert(sd_bus_wait(bus,10000)>=0);}
    sd_bus_unref(bus);
}
int main() {
    assert(geteuid()!=0);std::thread auth(server,true),service(server,false);
    for(unsigned i=0;i<1000 && (!ready_auth || !ready_service);++i)std::this_thread::sleep_for(std::chrono::milliseconds(1));
    assert(ready_auth && ready_service);
    sd_bus* bus=nullptr;assert(sd_bus_open_user(&bus)>=0);
    auto call=[&](const char* method,const char* color,unsigned percent,bool accept) {
        sd_bus_message* reply=nullptr;sd_bus_error error=SD_BUS_ERROR_NULL;
        int r=color?sd_bus_call_method(bus,uke::rgb::bus_name,uke::rgb::object_path,uke::rgb::interface,method,&error,&reply,"su",color,percent):
            sd_bus_call_method(bus,uke::rgb::bus_name,uke::rgb::object_path,uke::rgb::interface,method,&error,&reply,"");
        assert((r>=0)==accept);
        if(accept){const char* text=nullptr;assert(sd_bus_message_read(reply,"s",&text)>0 && std::string(text).find("org.uke.control.rgb.v1")!=std::string::npos);}
        sd_bus_error_free(&error);sd_bus_message_unref(reply);
    };
    call("Status",nullptr,0,true);
    fake_status.abi=99;call("Status",nullptr,0,false);fake_status.abi=UKE_RGB_ABI;
    fake_status.brightness=9;call("Status",nullptr,0,false);fake_status.brightness=0;
    fake_status.last_io_error=-4096;call("Status",nullptr,0,false);fake_status.last_io_error=0;
    allowed=false;call("SetColor","#ffff00",100,false);call("TestColors",nullptr,0,false);
    allowed=true;call("SetColor","#ff;000",100,false);call("SetColor","#ffff00",101,false);
    call("SetColor","#ffff00",100,true);call("TestColors",nullptr,0,true);call("TestColors",nullptr,0,false);
    call("Automatic",nullptr,0,true);call("Status",nullptr,0,true);
    auto torch_call=[&](const char* method,unsigned value,bool accept) {
        sd_bus_message* reply=nullptr;sd_bus_error error=SD_BUS_ERROR_NULL;
        const int r=sd_bus_call_method(bus,uke::torch::bus_name,uke::torch::object_path,uke::torch::interface,method,&error,&reply,
            std::string(method)=="SetEnabled"?"b":"u",value);
        assert((r>=0)==accept);
        if(accept){const char* text=nullptr;assert(sd_bus_message_read(reply,"s",&text)>0 && std::string(text).find("org.uke.control.lights.v1")!=std::string::npos);}
        sd_bus_error_free(&error);sd_bus_message_unref(reply);
    };
    allowed=false;torch_call("SetEnabled",1,false);torch_call("SetBrightness",50,false);
    allowed=true;torch_call("SetBrightness",101,false);torch_call("SetBrightness",1,true);
    torch_call("SetBrightness",99,true);torch_call("SetBrightness",100,true);torch_call("SetEnabled",0,true);
    sd_bus_unref(bus);running=false;auth.join();service.join();
    assert(changes==4); // Set, test, refused duplicate test, automatic; no unauthorized ioctl.
    assert((torch_fixture::writes==std::vector<unsigned>{0,1,1,2,0,0}));
    {
        State timed;timed.torch=std::make_unique<uke::torch::Actuator>();
        timed.torch->enable(true);timed.lease.applied(true,100);
        expire_torch(timed,60000099);assert(torch_fixture::brightness==2);
        expire_torch(timed,60000100);assert(torch_fixture::brightness==0);
        assert(timed.lease.remaining(60000100)==0);
        timed.torch->enable(true);timed.lease.applied(true,100);
        torch_fixture::fail_level=0;
        expire_torch(timed,60000100);
        assert(timed.off_pending && timed.lease.expired(60000100));
        const auto count=torch_fixture::writes.size();expire_torch(timed,60000101);
        assert(torch_fixture::writes.size()==count);
        torch_fixture::fail_level=9;expire_torch(timed,61000100);
        assert(torch_fixture::brightness==0 && !timed.off_pending);
        torch_fixture::bad_readback=true;torch_fixture::fail_level=0;
        bool failed=false;try{change_torch(timed,true,100,true);}catch(...){failed=true;}
        assert(failed && torch_fixture::brightness==2 && timed.off_pending);
        assert(timed.lease.remaining(boottime())>0);
        const auto uncertain_writes=torch_fixture::writes.size();
        failed=false;try{change_torch(timed,false,0,true);}catch(...){failed=true;}
        assert(failed && torch_fixture::writes.size()==uncertain_writes);
        torch_fixture::bad_readback=false;torch_fixture::fail_level=9;
        expire_torch(timed,timed.retry_after);
        assert(torch_fixture::brightness==0 && !timed.off_pending && !timed.lease.remaining(boottime()));
    }
    std::cout<<"Actual LED brokers: private-bus caller authorization, no-interaction Polkit, HEX/torch bounds and denied-request isolation passed; simulated hardware only\n";
}
