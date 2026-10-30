// SPDX-License-Identifier: MIT
#include "torch.hpp"
#include "cli.hpp"
#include "../shared/uke-torch.h"
#include <cerrno>
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
namespace uke::torch {
namespace {
void need(bool ok,const std::string& text){if(!ok)throw std::runtime_error(text);}
struct Fd {int n=-1;~Fd(){if(n>=0)close(n);}int release(){int value=n;n=-1;return value;}};
bool labeled(int fd,const char* expected) {
    char* label=nullptr;const int r=fgetfilecon(fd,&label);
    context_t parsed=r>=0?context_new(label):nullptr;
    const char* type=parsed?context_type_get(parsed):nullptr;
    const bool ok=type && std::string_view(type)==expected;
    if(parsed)context_free(parsed);
    if(label)freecon(label);
    return ok;
}
uke_torch_status read_status(int fd) {
    uke_torch_status value{};
    need(ioctl(fd,UKE_TORCH_STATUS,&value)==0,"Kernel torch status unavailable");
    need(value.abi==UKE_TORCH_ABI && value.requested_percent>=1 && value.requested_percent<=100 &&
         value.brightness<=2 && value.brightness_valid<=1 && value.max_microamp==UKE_TORCH_MAX_UA &&
         value.suspended<=1 && value.last_io_error<=0 && value.last_io_error>=-4095 && !value.reserved,
         "Kernel torch status failed ABI validation");
    return value;
}
}
Actuator::Actuator() {
    const auto* user=getpwnam("uke-rgb");
    need(user && user->pw_uid>0 && geteuid()==user->pw_uid,"Run the torch through its dedicated service");
    Fd device{open("/dev/uke-torch",O_RDWR|O_NOFOLLOW|O_CLOEXEC)};
    struct stat info{};
    need(device.n>=0 && !fstat(device.n,&info) && S_ISCHR(info.st_mode) &&
         info.st_uid==0 && info.st_gid==getegid() && (info.st_mode&0777)==0660 &&
         labeled(device.n,"uke_torch_device_t"),"Kernel torch device lacks the admitted ownership or SELinux label");
    (void)read_status(device.n);
    fd=device.release();
}
Actuator::~Actuator() {
    if(fd>=0)close(fd);
}
void Actuator::request(unsigned command,unsigned value) {
    const uke_torch_request input{UKE_TORCH_ABI,value,{0,0}};
    if(ioctl(fd,command,&input)<0)
        throw std::runtime_error("Kernel torch request refused: "+std::string(std::strerror(errno)));
}
void Actuator::set(unsigned percent) {
    need(percent<=100,"Torch percentage exceeds 100");
    request(UKE_TORCH_SET,percent);
}
void Actuator::enable(bool on){request(UKE_TORCH_ENABLE,on?1:0);}
std::string Actuator::status() {
    const auto value=read_status(fd);
    return "{\"schema\":\"org.uke.control.lights.v1\",\"physical_acceptance\":null,\"flashlight\":{\"available\":true,\"policy_owner\":\"kernel\",\"on\":"+
        std::string(value.brightness_valid?(value.brightness?"true":"false"):"null")+
        ",\"brightness_percent\":"+(value.brightness_valid?std::to_string(value.brightness*50):"null")+
        ",\"requested_percent\":"+std::to_string(value.requested_percent)+
        ",\"max_microamp\":"+std::to_string(value.max_microamp)+",\"brightness_step_percent\":50,\"max_seconds\":null,\"suspended\":"+
        (value.suspended?"true":"false")+",\"kernel_io_error\":"+std::to_string(value.last_io_error)+
        ",\"reason\":null},\"rgb\":{\"available\":false,\"on\":null,\"brightness_percent\":null}}\n";
}
int command(int argc,char** argv,bool as_json) {
    const bool status=argc==1 && std::string_view(argv[0])=="status";
    const bool on=argc==1 && std::string_view(argv[0])=="on";
    const bool off=argc==1 && std::string_view(argv[0])=="off";
    unsigned percent=0;
    if(!status && !on && !off) {
        need(argc==2 && std::string_view(argv[0])=="set","Usage: uke-control flashlight status|on|off|set 0..100");
        percent=lights::percent(argv[1]);
    }
    if(!status)need(geteuid()!=0,"Run torch requests as the active desktop user; sudo is not needed");
    sd_bus* raw=nullptr;need(sd_bus_open_system(&raw)>=0,"System bus unavailable");
    std::unique_ptr<sd_bus,decltype(&sd_bus_unref)> bus(raw,sd_bus_unref);
    sd_bus_set_method_call_timeout(raw,3000000);
    sd_bus_message* answer=nullptr;sd_bus_error error=SD_BUS_ERROR_NULL;
    const char* method=status?"Status":on||off?"SetEnabled":"SetBrightness";
    const int result=status?sd_bus_call_method(raw,bus_name,object_path,interface,method,&error,&answer,""):
        on||off?sd_bus_call_method(raw,bus_name,object_path,interface,method,&error,&answer,"b",on?1:0):
        sd_bus_call_method(raw,bus_name,object_path,interface,method,&error,&answer,"u",percent);
    std::unique_ptr<sd_bus_message,decltype(&sd_bus_message_unref)> reply(answer,sd_bus_message_unref);
    const std::string message=error.message?error.message:"Torch service unavailable";sd_bus_error_free(&error);
    if(result<0) {
        if(status){uke::cli::print("{\"schema\":\"org.uke.control.lights.v1\",\"physical_acceptance\":null,\"flashlight\":{\"available\":false,\"on\":null,\"brightness_percent\":null,\"requested_percent\":null,\"reason\":\"service_unavailable\"},\"rgb\":{\"available\":false,\"on\":null,\"brightness_percent\":null}}\n",as_json,"Flashlight");return 0;}
        throw std::runtime_error(message);
    }
    const char* text=nullptr;need(sd_bus_message_read(answer,"s",&text)>0 && text,"Invalid torch service response");
    uke::cli::print(text,as_json,"Flashlight");return 0;
}
}
