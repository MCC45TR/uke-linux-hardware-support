// SPDX-License-Identifier: MIT
// Only transports authorized requests. The kernel owns policy and animation.
#include "rgb.hpp"
#include "torch.hpp"
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
#include <time.h>
#include <unistd.h>
namespace {
void need(bool ok,const std::string& error){if(!ok)throw std::runtime_error(error);}
struct Pending {
    sd_bus_message* request=nullptr;
    sd_bus_slot* slot=nullptr;
    unsigned command=UKE_RGB_AUTOMATIC,value=0;
    uke_rgb_color color{};
    bool torch_enable=false;
    std::string sender,cancellation;
    ~Pending(){sd_bus_slot_unref(slot);sd_bus_message_unref(request);}
};
static constexpr uint64_t authorization_ms=120000;
struct State {
    int fd=-1;
    std::unique_ptr<uke::torch::Actuator> torch;
    std::unique_ptr<Pending> pending;
    std::string authorized_sender;
    uint64_t authorized_until=0;
};
uint64_t authorization_now_ms() {
    timespec value{};
    need(clock_gettime(CLOCK_BOOTTIME,&value)==0 && value.tv_sec>=0 &&
        uint64_t(value.tv_sec)<UINT64_MAX/1000-1,"Authorization clock unavailable");
    return uint64_t(value.tv_sec)*1000+uint64_t(value.tv_nsec)/1000000;
}
const char* mode_name(unsigned n) {
    static const char* names[]={"off","manual","low-battery","critical-battery","front-camera","test"};
    need(n<=UKE_RGB_TEST,"Invalid kernel RGB mode");return names[n];
}
std::string status(State& state) {
    const uint64_t now=authorization_now_ms();
    const auto remaining=state.authorized_until>now?state.authorized_until-now:0;
    const std::string session=",\"authorization_remaining_ms\":"+std::to_string(remaining);
    if(state.torch) {
        auto result=state.torch->status();result.insert(result.rfind('}'),session);return result;
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
    const auto channels=[](unsigned color){return "["+std::to_string(color>>16)+","+
        std::to_string((color>>8)&255)+","+std::to_string(color&255)+"]";};
    return "{\"schema\":\"org.uke.control.rgb.v1\",\"available\":true,\"physical_acceptance\":null,\"mode\":\""+
        std::string(mode_name(value.mode))+"\",\"color\":\""+c+"\",\"brightness_percent\":"+
        std::to_string((value.brightness*100+4)/8)+",\"color_rgb\":"+channels(value.rgb)+
        ",\"requested_color_rgb\":"+channels(value.requested_rgb)+",\"requested_percent\":"+std::to_string(value.requested_percent)+
        ",\"native_brightness\":"+std::to_string(value.brightness)+",\"native_brightness_limit\":"+std::to_string(value.brightness_limit)+
        ",\"policy_owner\":\"kernel\",\"test_remaining_ms\":"+std::to_string(value.test_remaining_ms)+
        ",\"battery_valid\":"+(value.battery_valid?"true":"false")+",\"camera_integration_available\":"+
        (value.camera_source_seen?"true":"false")+",\"kernel_io_error\":"+std::to_string(value.last_io_error)+",\"reason\":null"+session+"}";
}
int apply(State& state,Pending& pending) {
    try {
        if(state.torch) {
            if(pending.torch_enable)state.torch->enable(pending.value);
            else state.torch->set(pending.value);
        } else if(ioctl(state.fd,pending.command,&pending.color)<0)
            throw std::runtime_error("Kernel RGB request refused: "+std::string(std::strerror(errno)));
        return sd_bus_reply_method_return(pending.request,"s",status(state).c_str());
    } catch(const std::exception& e) {
        state.authorized_sender.clear();state.authorized_until=0;
        return sd_bus_reply_method_errorf(pending.request,"org.senemos.Uke.Rgb.Error","%s",e.what());
    }
}
int authorization_reply(sd_bus_message* reply,void* userdata,sd_bus_error*) {
    auto& state=*static_cast<State*>(userdata);
    auto pending=std::move(state.pending);
    if(!pending)return 0;
    int allowed=0,challenge=0;
    if(sd_bus_message_is_method_error(reply,nullptr)>0 ||
       sd_bus_message_enter_container(reply,'r',"bba{ss}")<=0 ||
       sd_bus_message_read(reply,"bb",&allowed,&challenge)<=0 || !allowed || challenge)
        return sd_bus_reply_method_errorf(pending->request,SD_BUS_ERROR_ACCESS_DENIED,
            "This hardware change requires a fresh system authorization");
    try {
        const uint64_t now=authorization_now_ms();
        need(now<=UINT64_MAX-authorization_ms,"Authorization clock overflow");
        state.authorized_sender=pending->sender;state.authorized_until=now+authorization_ms;
        return apply(state,*pending);
    } catch(const std::exception& e) {
        return sd_bus_reply_method_errorf(pending->request,"org.senemos.Uke.Rgb.Error","%s",e.what());
    }
}
int authorize(sd_bus_message* message,State& state,std::unique_ptr<Pending> pending,
              const char* action,const std::string& description,sd_bus_error* error) {
    sd_bus_creds* raw=nullptr;
    if(sd_bus_query_sender_creds(message,SD_BUS_CREDS_EUID,&raw)<0)
        return sd_bus_error_set_const(error,SD_BUS_ERROR_ACCESS_DENIED,"Caller credentials unavailable");
    std::unique_ptr<sd_bus_creds,decltype(&sd_bus_creds_unref)> creds(raw,sd_bus_creds_unref);
    uid_t uid=0;
    const char* sender=sd_bus_message_get_sender(message);
    uint64_t serial=0;
    if(sd_bus_creds_get_euid(raw,&uid)<0 || uid==0 || !sender || sender[0]!=':' || sd_bus_message_get_cookie(message,&serial)<0)
        return sd_bus_error_set_const(error,SD_BUS_ERROR_ACCESS_DENIED,"A non-root bus caller is required");
    pending->sender=sender;
    pending->request=sd_bus_message_ref(message);
    if(state.authorized_sender==sender && authorization_now_ms()<state.authorized_until)
        return apply(state,*pending);
    if(state.pending)return sd_bus_error_set_const(error,SD_BUS_ERROR_LIMITS_EXCEEDED,"Another hardware change is awaiting authorization");
    if(sd_bus_message_get_allow_interactive_authorization(message)<=0)
        return sd_bus_error_set_const(error,SD_BUS_ERROR_INTERACTIVE_AUTHORIZATION_REQUIRED,
            "Allow a system authorization for this two-minute control session");
    pending->cancellation="uke-light-"+pending->sender+"-"+std::to_string(serial);
    sd_bus* bus=sd_bus_message_get_bus(message);
    sd_bus_message* call=nullptr;
    need(sd_bus_message_new_method_call(bus,&call,"org.freedesktop.PolicyKit1",
        "/org/freedesktop/PolicyKit1/Authority","org.freedesktop.PolicyKit1.Authority","CheckAuthorization")>=0,
        "Cannot prepare system authorization");
    std::unique_ptr<sd_bus_message,decltype(&sd_bus_message_unref)> outgoing(call,sd_bus_message_unref);
    need(sd_bus_message_append(call,"(sa{sv})sa{ss}us","system-bus-name",1,"name","s",sender,
        action,1,"polkit.message",(description+". Allow this application to adjust this light for two minutes.").c_str(),
        1U,pending->cancellation.c_str())>=0,"Cannot encode system authorization");
    state.pending=std::move(pending);
    const int r=sd_bus_call_async(bus,&state.pending->slot,call,authorization_reply,&state,120000000);
    if(r<0){state.pending.reset();return sd_bus_error_set_const(error,SD_BUS_ERROR_ACCESS_DENIED,"System authorization unavailable");}
    return 1;
}
int caller_disconnected(sd_bus_message* message,void* userdata,sd_bus_error*) {
    auto& state=*static_cast<State*>(userdata);
    const char *name=nullptr,*old_owner=nullptr,*new_owner=nullptr;
    if(sd_bus_message_read(message,"sss",&name,&old_owner,&new_owner)<=0 || !name || !new_owner || *new_owner)return 0;
    if(state.authorized_sender==name){state.authorized_sender.clear();state.authorized_until=0;}
    if(!state.pending || state.pending->sender!=name)return 0;
    auto pending=std::move(state.pending);
    sd_bus_message* call=nullptr;
    if(sd_bus_message_new_method_call(sd_bus_message_get_bus(message),&call,"org.freedesktop.PolicyKit1",
        "/org/freedesktop/PolicyKit1/Authority","org.freedesktop.PolicyKit1.Authority","CancelCheckAuthorization")>=0) {
        if(sd_bus_message_append(call,"s",pending->cancellation.c_str())>=0 && sd_bus_message_set_expect_reply(call,0)>=0)
            (void)sd_bus_send(sd_bus_message_get_bus(message),call,nullptr);
        sd_bus_message_unref(call);
    }
    return 0;
}
int request(sd_bus_message* message,void* userdata,sd_bus_error* error) {
    auto& state=*static_cast<State*>(userdata);
    try {
        const std::string method=sd_bus_message_get_member(message);
        if(method=="Status")return sd_bus_reply_method_return(message,"s",status(state).c_str());
        auto pending=std::make_unique<Pending>();
        if(state.torch) {
            unsigned percent=0;int enabled=0;
            if(method=="SetBrightness")need(sd_bus_message_read(message,"u",&percent)>0 && percent<=100,"Invalid torch percentage");
            else need(method=="SetEnabled" && sd_bus_message_read(message,"b",&enabled)>0,"Invalid torch request");
            pending->torch_enable=method=="SetEnabled";
            pending->value=pending->torch_enable?unsigned(enabled):percent;
            const std::string description=pending->torch_enable?(enabled?"Turn on the Uke flashlight":"Turn off the Uke flashlight"):
                "Set the Uke flashlight request to "+std::to_string(percent)+" percent";
            return authorize(message,state,std::move(pending),uke::torch::action,description,error);
        }
        uke_rgb_color value{};unsigned command=UKE_RGB_AUTOMATIC;
        if(method=="SetColor") {
            const char* text=nullptr;unsigned percent=0;
            need(sd_bus_message_read(message,"su",&text,&percent)>0 && text && percent<=100,"Invalid RGB request");
            const auto color=uke::rgb::parse_hex(text);
            value={UKE_RGB_ABI,(color[0]<<16)|(color[1]<<8)|color[2],percent,0};command=UKE_RGB_SET;
        } else if(method=="SetRgb") {
            unsigned r=0,g=0,b=0,percent=0;
            need(sd_bus_message_read(message,"uuuu",&r,&g,&b,&percent)>0 &&
                r<=255 && g<=255 && b<=255 && percent<=100,"Invalid RGB channels or percentage");
            value={UKE_RGB_ABI,(r<<16)|(g<<8)|b,percent,0};command=UKE_RGB_SET;
        } else if(method=="TestColors")command=UKE_RGB_TEST_COLORS;
        else need(method=="Automatic","Unknown RGB method");
        pending->command=command;pending->color=value;
        const std::string description=command==UKE_RGB_SET?"Set the Uke notification LED to "+
            uke::rgb::hex({value.rgb>>16,(value.rgb>>8)&255,value.rgb&255})+" at "+std::to_string(value.brightness_percent)+" percent":
            method=="TestColors"?"Run the Uke notification LED color test":"Restore automatic Uke notification LED policy";
        return authorize(message,state,std::move(pending),"org.senemos.uke.rgb.control",description,error);
    } catch(const std::exception& e){return sd_bus_error_set(error,"org.senemos.Uke.Rgb.Error",e.what());}
}
const sd_bus_vtable methods[]={
    SD_BUS_VTABLE_START(0),
    SD_BUS_METHOD("Status","","s",request,SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("SetColor","su","s",request,SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("SetRgb","uuuu","s",request,SD_BUS_VTABLE_UNPRIVILEGED),
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
        const bool torch=argc==2 && std::string_view(argv[1])=="--torch";
        need(argc==1 || torch,"Unknown LED service mode");
        require_confinement();
        if(torch)state.torch=std::make_unique<uke::torch::Actuator>();
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
        need(sd_bus_match_signal(raw,nullptr,"org.freedesktop.DBus","/org/freedesktop/DBus",
            "org.freedesktop.DBus","NameOwnerChanged",caller_disconnected,&state)>=0,"Cannot track request caller lifetime");
        need(sd_bus_request_name(raw,torch?uke::torch::bus_name:uke::rgb::bus_name,0)>=0,"Another LED broker owns the interface");
        signal(SIGTERM,stop);signal(SIGINT,stop);
        while(!stopping) {
            int r;do{r=sd_bus_process(raw,nullptr);need(r>=0,"RGB bus disconnected");}while(r>0);
            const int waiting=sd_bus_wait(raw,1000000);
            need(waiting>=0 || waiting==-EINTR,"Cannot wait for RGB requests");
        }
        if(state.fd>=0)close(state.fd);
        return 0;
    } catch(const std::exception& e){if(state.fd>=0)close(state.fd);std::cerr<<e.what()<<'\n';return 1;}
}
