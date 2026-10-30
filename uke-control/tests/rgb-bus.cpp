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
static std::atomic<uint64_t> boottime_ms{1000};
static std::atomic<bool> clock_error{false};
static int test_clock_gettime(clockid_t clock,timespec* value) {
    assert(clock==CLOCK_BOOTTIME);if(clock_error)return -1;
    value->tv_sec=boottime_ms/1000;value->tv_nsec=(boottime_ms%1000)*1000000;return 0;
}
static int test_ioctl(int fd,unsigned long command,uke_rgb_status* arg) {
    assert(fd==91 && command==UKE_RGB_STATUS);*arg=fake_status;return 0;
}
static int test_ioctl(int fd,unsigned long command,uke_rgb_color* arg) {
    assert(fd==91 && command!=UKE_RGB_STATUS);
    ++changes;
    if(command==UKE_RGB_SET) {
        auto c=*arg;assert(c.abi==1 && c.rgb<=0xffffff && c.brightness_percent<=100 && !c.reserved);
        fake_status.mode=UKE_RGB_MANUAL;fake_status.rgb=c.rgb;fake_status.requested_rgb=c.rgb;
        fake_status.brightness=c.brightness_percent*8/100;
    } else if(command==UKE_RGB_TEST_COLORS) {
        if(fake_status.mode==UKE_RGB_TEST){errno=EBUSY;return -1;}
        fake_status.mode=UKE_RGB_TEST;fake_status.test_remaining_ms=10000;
    } else {assert(command==UKE_RGB_AUTOMATIC);fake_status.mode=UKE_RGB_OFF;fake_status.test_remaining_ms=0;}
    return 0;
}
#define ioctl test_ioctl
#define clock_gettime test_clock_gettime
#define main rgb_broker_main
#include "../src/rgb-service.cpp"
#undef main
#undef ioctl
#undef clock_gettime
static std::atomic<bool> running{true},allowed{true},ready_auth{false},ready_service{false};
static std::atomic<bool> challenge{false},authority_error{false},hold{false},held{false},release_auth{false},cancelled{false};
static std::atomic<unsigned> authorizations{0};
static sd_bus_message* held_request=nullptr; // Used only by the authority thread.
static int authorize_mock(sd_bus_message* m,void*,sd_bus_error*) {
    assert(sd_bus_message_enter_container(m,'r',"sa{sv}")>0);
    const char* subject=nullptr;assert(sd_bus_message_read(m,"s",&subject)>0 && std::string(subject)=="system-bus-name");
    assert(sd_bus_message_skip(m,"a{sv}")>0);assert(sd_bus_message_exit_container(m)>0);
    const char* action=nullptr;assert(sd_bus_message_read(m,"s",&action)>0 &&
        (std::string(action)=="org.senemos.uke.rgb.control" || std::string(action)==uke::torch::action));
    assert(sd_bus_message_enter_container(m,'a',"{ss}")>0);
    assert(sd_bus_message_enter_container(m,'e',"ss")>0);
    const char *key=nullptr,*description=nullptr;
    assert(sd_bus_message_read(m,"ss",&key,&description)>0 && std::string(key)=="polkit.message" && std::string(description).find("Uke")!=std::string::npos);
    assert(sd_bus_message_exit_container(m)>0 && sd_bus_message_exit_container(m)>0);
    unsigned flags=99;const char* cancel=nullptr;
    assert(sd_bus_message_read(m,"us",&flags,&cancel)>0 && flags==1 && std::string(cancel).starts_with("uke-light-:"));
    ++authorizations;
    if(hold){assert(!held_request);held_request=sd_bus_message_ref(m);held=true;return 1;}
    if(authority_error)return sd_bus_reply_method_errorf(m,SD_BUS_ERROR_ACCESS_DENIED,"No authentication agent");
    return sd_bus_reply_method_return(m,"(bba{ss})",allowed.load()?1:0,challenge.load()?1:0,0);
}
static int cancel_mock(sd_bus_message* m,void*,sd_bus_error*) {
    const char* id=nullptr;assert(sd_bus_message_read(m,"s",&id)>0 && std::string(id).starts_with("uke-light-:"));
    cancelled=true;return sd_bus_reply_method_return(m,"");
}
static const sd_bus_vtable auth_methods[]={SD_BUS_VTABLE_START(0),
    SD_BUS_METHOD("CheckAuthorization","(sa{sv})sa{ss}us","(bba{ss})",authorize_mock,SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("CancelCheckAuthorization","s","",cancel_mock,SD_BUS_VTABLE_UNPRIVILEGED),SD_BUS_VTABLE_END};
static void server(bool auth) {
    sd_bus* bus=nullptr;assert(sd_bus_open_user(&bus)>=0);State state,torch;state.fd=91;
    if(auth) {
        assert(sd_bus_add_object_vtable(bus,nullptr,"/org/freedesktop/PolicyKit1/Authority","org.freedesktop.PolicyKit1.Authority",auth_methods,nullptr)>=0);
        assert(sd_bus_request_name(bus,"org.freedesktop.PolicyKit1",0)>=0);ready_auth=true;
    } else {
        torch.torch=std::make_unique<uke::torch::Actuator>();
        assert(sd_bus_add_object_vtable(bus,nullptr,uke::rgb::object_path,uke::rgb::interface,methods,&state)>=0);
        assert(sd_bus_add_object_vtable(bus,nullptr,uke::torch::object_path,uke::torch::interface,torch_methods,&torch)>=0);
        for(State* owner:{&state,&torch})assert(sd_bus_match_signal(bus,nullptr,"org.freedesktop.DBus","/org/freedesktop/DBus",
            "org.freedesktop.DBus","NameOwnerChanged",caller_disconnected,owner)>=0);
        assert(sd_bus_request_name(bus,uke::torch::bus_name,0)>=0);
        assert(sd_bus_request_name(bus,uke::rgb::bus_name,0)>=0);ready_service=true;
    }
    while(running) {
        int r;do{r=sd_bus_process(bus,nullptr);assert(r>=0);}while(r>0);
        if(auth && held_request && release_auth.exchange(false)) {
            assert(sd_bus_reply_method_return(held_request,"(bba{ss})",1,0,0)>=0);
            sd_bus_message_unref(held_request);held_request=nullptr;held=false;
        }
        assert(sd_bus_wait(bus,10000)>=0);
    }
    sd_bus_unref(bus);
}
int main() {
    assert(geteuid()!=0);std::thread auth(server,true),service(server,false);
    for(unsigned i=0;i<1000 && (!ready_auth || !ready_service);++i)std::this_thread::sleep_for(std::chrono::milliseconds(1));
    assert(ready_auth && ready_service);
    sd_bus* bus=nullptr;assert(sd_bus_open_user(&bus)>=0);
    assert(sd_bus_set_allow_interactive_authorization(bus,1)>=0);
    auto call=[&](const char* method,const char* color,unsigned percent,bool accept) {
        sd_bus_message* reply=nullptr;sd_bus_error error=SD_BUS_ERROR_NULL;
        int r=color?sd_bus_call_method(bus,uke::rgb::bus_name,uke::rgb::object_path,uke::rgb::interface,method,&error,&reply,"su",color,percent):
            sd_bus_call_method(bus,uke::rgb::bus_name,uke::rgb::object_path,uke::rgb::interface,method,&error,&reply,"");
        assert((r>=0)==accept);
        if(accept){const char* text=nullptr;assert(sd_bus_message_read(reply,"s",&text)>0 && std::string(text).find("org.uke.control.rgb.v1")!=std::string::npos);}
        sd_bus_error_free(&error);sd_bus_message_unref(reply);
    };
    call("Status",nullptr,0,true);
    assert(sd_bus_set_allow_interactive_authorization(bus,0)>=0);
    call("SetColor","#ffff00",100,false);assert(authorizations==0 && changes==0);
    assert(sd_bus_set_allow_interactive_authorization(bus,1)>=0);
    fake_status.abi=99;call("Status",nullptr,0,false);fake_status.abi=UKE_RGB_ABI;
    fake_status.brightness=9;call("Status",nullptr,0,false);fake_status.brightness=0;
    fake_status.last_io_error=-4096;call("Status",nullptr,0,false);fake_status.last_io_error=0;
    allowed=false;call("SetColor","#ffff00",100,false);call("TestColors",nullptr,0,false);
    allowed=true;call("SetColor","#ff;000",100,false);call("SetColor","#ffff00",101,false);
    authority_error=true;call("SetColor","#ffff00",100,false);authority_error=false;
    challenge=true;call("SetColor","#ffff00",100,false);challenge=false;
    call("SetColor","#ffff00",100,true);call("TestColors",nullptr,0,true);call("TestColors",nullptr,0,false);
    call("Automatic",nullptr,0,true);call("Status",nullptr,0,true);
    // A second connection under the same UID cannot use the first one's lease.
    sd_bus* stranger=nullptr;assert(sd_bus_open_user(&stranger)>=0);
    sd_bus_message* denial=nullptr;sd_bus_error denied=SD_BUS_ERROR_NULL;
    assert(sd_bus_call_method(stranger,uke::rgb::bus_name,uke::rgb::object_path,uke::rgb::interface,"Automatic",&denied,&denial,"")<0);
    sd_bus_error_free(&denied);sd_bus_message_unref(denial);sd_bus_unref(stranger);assert(changes==4);
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
    // A held authorization cannot block status or authorize a different request.
    boottime_ms+=120001;hold=true;sd_bus_message* outgoing=nullptr;sd_bus_slot* slot=nullptr;bool done=false;
    assert(sd_bus_message_new_method_call(bus,&outgoing,uke::rgb::bus_name,uke::rgb::object_path,uke::rgb::interface,"TestColors")>=0);
    assert(sd_bus_call_async(bus,&slot,outgoing,[](sd_bus_message* m,void* p,sd_bus_error*){
        assert(sd_bus_message_is_method_error(m,nullptr)==0);*static_cast<bool*>(p)=true;return 1;
    },&done,3000000)>=0);sd_bus_message_unref(outgoing);sd_bus_flush(bus);
    for(unsigned i=0;i<1000 && !held;++i)std::this_thread::sleep_for(std::chrono::milliseconds(1));
    assert(held && !done && changes==4);const unsigned count=authorizations;
    call("Status",nullptr,0,true);call("SetColor","#00ff00",50,false);assert(authorizations==count && changes==4);
    hold=false;release_auth=true;
    for(unsigned i=0;i<1000 && !done;++i){assert(sd_bus_process(bus,nullptr)>=0);sd_bus_wait(bus,1000);}
    assert(done && changes==5);sd_bus_slot_unref(slot);
    // Dropping a caller cancels its request; a late positive reply cannot apply it.
    sd_bus* disappearing=nullptr;assert(sd_bus_open_user(&disappearing)>=0);
    assert(sd_bus_set_allow_interactive_authorization(disappearing,1)>=0);hold=true;cancelled=false;
    assert(sd_bus_message_new_method_call(disappearing,&outgoing,uke::rgb::bus_name,uke::rgb::object_path,uke::rgb::interface,"Automatic")>=0);
    assert(sd_bus_call_async(disappearing,&slot,outgoing,nullptr,nullptr,3000000)>=0);
    sd_bus_message_unref(outgoing);sd_bus_flush(disappearing);
    for(unsigned i=0;i<1000 && !held;++i)std::this_thread::sleep_for(std::chrono::milliseconds(1));
    assert(held);
    sd_bus_slot_unref(slot);sd_bus_close(disappearing);sd_bus_unref(disappearing);
    for(unsigned i=0;i<1000 && !cancelled;++i)std::this_thread::sleep_for(std::chrono::milliseconds(1));
    assert(cancelled && changes==5);
    hold=false;release_auth=true;
    for(unsigned i=0;i<1000 && held;++i)std::this_thread::sleep_for(std::chrono::milliseconds(1));
    call("Status",nullptr,0,true);assert(changes==5);
    const auto checks=authorizations.load();
    // Requests never extend the fixed deadline, and clock failure denies writes.
    boottime_ms+=119999;clock_error=true;call("SetColor","#ffffff",50,false);clock_error=false;
    call("Automatic",nullptr,0,true);assert(authorizations==checks && changes==6);
    boottime_ms+=2;allowed=false;call("Automatic",nullptr,0,false);assert(authorizations==checks+1 && changes==6);
    sd_bus_unref(bus);running=false;auth.join();service.join();
    assert(changes==6); // Original calls, an authorized held test and a bounded lease request.
    assert((torch_fixture::requests==std::vector<std::pair<unsigned long,unsigned>>{
        {UKE_TORCH_SET,1},{UKE_TORCH_SET,99},{UKE_TORCH_SET,100},{UKE_TORCH_ENABLE,0}}));
    std::cout<<"Actual LED brokers: connection-bound expiring authorization, same-UID peer isolation, denied/missing-agent handling, responsive status, caller cancellation and rejected late replies passed; simulated hardware only\n";
}
