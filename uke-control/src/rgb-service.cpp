// SPDX-License-Identifier: MIT
// Only transports authorized requests. The kernel owns policy and animation.
#include "rgb.hpp"
#include "torch.hpp"
#include "torch-lease.hpp"
#include <time.h>
#include "../shared/uke-rgb.h"
#include <cerrno>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <memory>
#include <pwd.h>
#include <selinux/context.h>
#include <selinux/selinux.h>
#include <stdexcept>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <systemd/sd-bus.h>
#include <unistd.h>
namespace {
void need(bool ok,const std::string& error){if(!ok)throw std::runtime_error(error);}
struct State {int fd=-1;std::unique_ptr<uke::torch::Actuator> torch;uke::torch::Lease lease;bool off_pending=false;uint64_t retry_after=0;};
uint64_t boottime() {
    timespec now{};need(clock_gettime(CLOCK_BOOTTIME,&now)==0,"Cannot read torch safety clock");
    return uint64_t(now.tv_sec)*1000000+uint64_t(now.tv_nsec)/1000;
}
void expire_torch(State& state,uint64_t now=boottime()) {
    if(!state.torch || (!state.off_pending && !state.lease.expired(now)) || now<state.retry_after)return;
    try {
        state.torch->enable(false); // includes kernel-provider readback, not just a successful write
        state.lease.applied(false,now);state.off_pending=false;state.retry_after=0;
    } catch(const std::exception& e) {
        state.off_pending=true;state.retry_after=now+1000000;
        std::cerr<<"Torch OFF remains unverified: "<<e.what()<<'\n';
    }
}
const char* mode_name(unsigned n) {
    static const char* names[]={"off","manual","low-battery","critical-battery","front-camera","test"};
    need(n<=UKE_RGB_TEST,"Invalid kernel RGB mode");return names[n];
}
std::string status(State& state) {
    if(state.torch) {
        auto value=state.torch->status();
        value.insert(1,"\"off_verification_pending\":"+std::string(state.off_pending?"true":"false")+",\"auto_off_remaining_ms\":"+std::to_string(state.lease.remaining(boottime())/1000)+",");
        return value;
    }
    uke_rgb_status value{};
    need(::ioctl(state.fd,UKE_RGB_STATUS,&value)==0,"Kernel RGB status unavailable");
    need(value.abi==UKE_RGB_ABI && value.mode<=UKE_RGB_TEST && value.rgb<=0xffffff &&
         value.brightness_limit==UKE_RGB_LIMIT && value.brightness<=UKE_RGB_LIMIT &&
         value.requested_rgb<=0xffffff && value.requested_percent<=100 && value.battery_valid<=1 &&
         (!value.battery_valid || value.battery_percent<=100) && value.test_remaining_ms<=UKE_RGB_TEST_MS &&
         value.camera_active<=1 && value.camera_source_seen<=1 && value.last_io_error<=0 && value.last_io_error>=-4095 && !value.reserved,
         "Kernel RGB status failed ABI validation");
    const auto c=uke::rgb::hex({value.rgb>>16,(value.rgb>>8)&255,value.rgb&255});
    return "{\"schema\":\"org.uke.control.rgb.v1\",\"available\":true,\"physical_acceptance\":null,\"mode\":\""+
        std::string(mode_name(value.mode))+"\",\"color\":\""+c+"\",\"brightness_percent\":"+
        std::to_string((value.brightness*100+4)/8)+",\"test_remaining_ms\":"+std::to_string(value.test_remaining_ms)+
        ",\"battery_valid\":"+(value.battery_valid?"true":"false")+",\"camera_integration_available\":"+
        (value.camera_source_seen?"true":"false")+",\"kernel_io_error\":"+std::to_string(value.last_io_error)+",\"reason\":null}";
}
bool authorized(sd_bus_message* message,const char* action) {
    sd_bus_creds* raw=nullptr;
    if(sd_bus_query_sender_creds(message,SD_BUS_CREDS_EUID,&raw)<0)return false;
    std::unique_ptr<sd_bus_creds,decltype(&sd_bus_creds_unref)> creds(raw,sd_bus_creds_unref);
    uid_t uid=0;
    if(sd_bus_creds_get_euid(raw,&uid)<0 || uid==0)return false;
    sd_bus_message* answer=nullptr;sd_bus_error error=SD_BUS_ERROR_NULL;
    const int r=sd_bus_call_method(sd_bus_message_get_bus(message),"org.freedesktop.PolicyKit1",
        "/org/freedesktop/PolicyKit1/Authority","org.freedesktop.PolicyKit1.Authority","CheckAuthorization",
        &error,&answer,"(sa{sv})sa{ss}us","system-bus-name",1,"name","s",sd_bus_message_get_sender(message),
        action,0,0U,"");
    std::unique_ptr<sd_bus_message,decltype(&sd_bus_message_unref)> reply(answer,sd_bus_message_unref);
    sd_bus_error_free(&error);
    int allowed=0,challenge=0;
    return r>=0 && sd_bus_message_enter_container(answer,'r',"bba{ss}")>0 &&
        sd_bus_message_read(answer,"bb",&allowed,&challenge)>0 && allowed && !challenge;
}
void change_torch(State& state,bool brightness,unsigned percent,bool enabled) {
    expire_torch(state);
    const bool on=brightness?percent!=0:enabled!=0;
    need(!on || !state.off_pending,"Torch OFF is not yet verified; new ON requests are blocked");
    // Arm before any potentially energizing write, including writes whose readback fails.
    if(on)state.lease.applied(true,boottime());
    try {
        if(brightness)state.torch->set(percent);else state.torch->enable(enabled);
        if(!on){state.lease.applied(false,boottime());state.off_pending=false;state.retry_after=0;}
    } catch(...) {
        state.off_pending=true;state.retry_after=boottime()+1000000;
        throw;
    }
}
int request(sd_bus_message* message,void* userdata,sd_bus_error* error) {
    auto& state=*static_cast<State*>(userdata);
    try {
        const std::string method=sd_bus_message_get_member(message);
        if(method=="Status")return sd_bus_reply_method_return(message,"s",status(state).c_str());
        if(state.torch) {
            unsigned percent=0;int enabled=0;
            if(method=="SetBrightness")need(sd_bus_message_read(message,"u",&percent)>0 && percent<=100,"Invalid torch percentage");
            else need(method=="SetEnabled" && sd_bus_message_read(message,"b",&enabled)>0,"Invalid torch request");
            if(!authorized(message,uke::torch::action))return sd_bus_error_set_const(error,SD_BUS_ERROR_ACCESS_DENIED,"Only an active local user may request torch changes");
            change_torch(state,method=="SetBrightness",percent,enabled);
            return sd_bus_reply_method_return(message,"s",status(state).c_str());
        }
        uke_rgb_color value{};unsigned command=UKE_RGB_AUTOMATIC;
        if(method=="SetColor") {
            const char* text=nullptr;unsigned percent=0;
            need(sd_bus_message_read(message,"su",&text,&percent)>0 && text && percent<=100,"Invalid RGB request");
            const auto color=uke::rgb::parse_hex(text);
            value={UKE_RGB_ABI,(color[0]<<16)|(color[1]<<8)|color[2],percent,0};command=UKE_RGB_SET;
        } else if(method=="TestColors")command=UKE_RGB_TEST_COLORS;
        else need(method=="Automatic","Unknown RGB method");
        if(!authorized(message,"org.senemos.uke.rgb.control"))return sd_bus_error_set_const(error,SD_BUS_ERROR_ACCESS_DENIED,"Only an active local user may request RGB changes");
        if(::ioctl(state.fd,command,&value)<0)throw std::runtime_error("Kernel RGB request refused: "+std::string(std::strerror(errno)));
        return sd_bus_reply_method_return(message,"s",status(state).c_str());
    } catch(const std::exception& e){return sd_bus_error_set(error,"org.senemos.Uke.Rgb.Error",e.what());}
}
const sd_bus_vtable methods[]={
    SD_BUS_VTABLE_START(0),
    SD_BUS_METHOD("Status","","s",request,SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("SetColor","su","s",request,SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("TestColors","","s",request,SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("Automatic","","s",request,SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_VTABLE_END
};
const sd_bus_vtable torch_methods[]={
    SD_BUS_VTABLE_START(0),
    SD_BUS_METHOD("Status","","s",request,SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("SetEnabled","b","s",request,SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("SetBrightness","u","s",request,SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_VTABLE_END
};
bool type_matches(const char* label,const char* expected) {
    context_t context=context_new(label);if(!context)return false;
    const char* type=context_type_get(context);
    const bool ok=type && std::string_view(type)==expected;context_free(context);return ok;
}
void require_confinement() {
    const auto* user=getpwnam("uke-rgb");
    need(user && user->pw_uid>0 && geteuid()==user->pw_uid,"Run RGB broker through its dedicated system service");
    need(is_selinux_enabled()==1 && security_getenforce()==1,"RGB broker requires SELinux enforcing");
    char* label=nullptr;int r=getcon(&label);
    const bool ok=r>=0 && label && type_matches(label,"uke_rgb_t");if(label)freecon(label);
    need(ok,"RGB broker is outside its confined domain");
}
volatile sig_atomic_t stopping=0;
void stop(int){stopping=1;}
}
int main(int argc,char** argv) {
    State state;
    try {
        const bool torch=argc==2 && (std::string_view(argv[1])=="--torch" || std::string_view(argv[1])=="--torch-off");
        need(argc==1 || torch,"Unknown LED service mode");
        require_confinement();
        if(torch)state.torch=std::make_unique<uke::torch::Actuator>();
        if(torch && std::string_view(argv[1])=="--torch-off")return 0;
        if(!torch) {
        state.fd=open("/dev/uke-rgb",O_RDWR|O_NOFOLLOW|O_CLOEXEC);
        struct stat info{};char* label=nullptr;
        const bool opened=state.fd>=0 && fstat(state.fd,&info)==0 && S_ISCHR(info.st_mode) &&
            info.st_uid==0 && info.st_gid==getegid() && (info.st_mode&0777)==0660 && fgetfilecon(state.fd,&label)>=0;
        const bool labeled=opened && label && type_matches(label,"uke_rgb_device_t");if(label)freecon(label);
        need(labeled,"Kernel RGB device lacks the admitted ownership or label");
        }
        (void)status(state);
        sd_bus* raw=nullptr;need(sd_bus_open_system(&raw)>=0,"System bus unavailable");
        std::unique_ptr<sd_bus,decltype(&sd_bus_unref)> bus(raw,sd_bus_unref);
        sd_bus_set_method_call_timeout(raw,2000000);
        need(sd_bus_add_object_vtable(raw,nullptr,torch?uke::torch::object_path:uke::rgb::object_path,
             torch?uke::torch::interface:uke::rgb::interface,torch?torch_methods:methods,&state)>=0,"Cannot register LED interface");
        need(sd_bus_request_name(raw,torch?uke::torch::bus_name:uke::rgb::bus_name,0)>=0,"Another LED broker owns the interface");
        signal(SIGTERM,stop);signal(SIGINT,stop);
        while(!stopping) {
            int r;do{expire_torch(state);r=sd_bus_process(raw,nullptr);need(r>=0,"RGB bus disconnected");}while(r>0 && !stopping);
            const int waiting=sd_bus_wait(raw,1000000);
            need(waiting>=0 || waiting==-EINTR,"Cannot wait for RGB requests");
        }
        if(state.fd>=0)close(state.fd);
        return 0;
    } catch(const std::exception& e){if(state.fd>=0)close(state.fd);std::cerr<<e.what()<<'\n';return 1;}
}
