// SPDX-License-Identifier: MIT
#include "torch.hpp"
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <grp.h>
#include <iostream>
#include <linux/magic.h>
#include <memory>
#include <pwd.h>
#include <selinux/context.h>
#include <selinux/selinux.h>
#include <stdexcept>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/vfs.h>
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
void sysfs_file(int fd) {
    struct statfs filesystem{};struct stat info{};
    need(fd>=0 && !fstatfs(fd,&filesystem) && filesystem.f_type==SYSFS_MAGIC &&
         !fstat(fd,&info) && S_ISREG(info.st_mode),"Torch target is not a regular sysfs attribute");
}
lights::Source admitted() {
    auto result=lights::discover(false);
    need(!result.path.empty(),result.reason);return result;
}
void write_level(int fd,unsigned level) {
    need(level<=2,"Torch current step exceeds the admitted 20 mA ceiling");
    need(lseek(fd,0,SEEK_SET)==0,"Cannot seek torch attribute");
    const std::string value=std::to_string(level)+"\n";
    ssize_t n;do{n=write(fd,value.data(),value.size());}while(n<0 && errno==EINTR);
    need(n==ssize_t(value.size()),"Torch write failed: "+std::string(std::strerror(errno)));
}
}
Actuator::Actuator() {
    const auto* user=getpwnam("uke-rgb");
    need(user && user->pw_uid>0 && geteuid()==user->pw_uid,"Run the torch through its dedicated service");
    source=admitted();
    Fd lock{open("/run/uke-control-torch/lock",O_CREAT|O_RDWR|O_NOFOLLOW|O_CLOEXEC,0600)};
    struct stat info{};
    need(lock.n>=0 && !fstat(lock.n,&info) && S_ISREG(info.st_mode) &&
         info.st_uid==geteuid() && info.st_nlink==1 && !(info.st_mode&0077),"Invalid torch ownership lock");
    need(flock(lock.n,LOCK_EX|LOCK_NB)==0,"Another torch writer is active");
    Fd brightness{open((source.path/"brightness").c_str(),O_RDWR|O_NOFOLLOW|O_CLOEXEC)};
    sysfs_file(brightness.n);
    need(!fstat(brightness.n,&info) && info.st_uid==0 && info.st_gid==getegid() &&
         (info.st_mode&0777)==0664 && labeled(brightness.n,"uke_torch_brightness_t"),
         "Torch attribute lacks the admitted ownership or SELinux label");
    write_level(brightness.n,0);
    need(admitted().brightness==0,"Torch startup-off readback differs");
    lock_fd=lock.release();brightness_fd=brightness.release();
}
Actuator::~Actuator() {
    if(brightness_fd>=0) {
        try{write_level(brightness_fd,0);}catch(const std::exception& e){std::cerr<<e.what()<<'\n';}
        close(brightness_fd);
    }
    if(lock_fd>=0)close(lock_fd);
}
void Actuator::apply(unsigned level) {
    try {
        need(admitted().path==source.path,"Torch provider changed");
        write_level(brightness_fd,level);
        need(admitted().brightness==level,"Torch brightness readback differs");
    } catch(...) {try{write_level(brightness_fd,0);}catch(...){}throw;}
}
void Actuator::set(unsigned percent) {
    need(percent<=100,"Torch percentage exceeds 100");
    if(percent)requested=percent;
    apply(lights::level(percent,2));
}
void Actuator::enable(bool on){apply(on?lights::level(requested,2):0);}
std::string Actuator::status() {
    need(admitted().path==source.path,"Torch provider changed");
    auto result=lights::snapshot();
    const auto position=result.find(",\"max_microamp\":");
    need(position!=std::string::npos,"Invalid torch status");
    result.insert(position,",\"requested_percent\":"+std::to_string(requested));return result;
}
int prepare_access(bool remove) {
    need(geteuid()==0,"Torch permission setup is reserved for package and device initialization");
    auto source=lights::discover(false);
    if(source.path.empty())return 0;
    need(is_selinux_enabled()==1 && security_getenforce()==1,"Torch permissions require SELinux Enforcing");
    const auto* group=getgrnam("uke-rgb");need(group && group->gr_gid>0,"LED service group unavailable");
    Fd fd{open((source.path/"brightness").c_str(),O_RDONLY|O_NOFOLLOW|O_CLOEXEC)};
    sysfs_file(fd.n);
    need(labeled(fd.n,"sysfs_t") || labeled(fd.n,"uke_torch_brightness_t"),"Unexpected torch attribute label");
    const char* context=remove?"system_u:object_r:sysfs_t:s0":"system_u:object_r:uke_torch_brightness_t:s0";
    need(fsetfilecon(fd.n,context)==0,"Cannot label the admitted torch attribute");
    need(fchown(fd.n,0,remove?0:group->gr_gid)==0 && fchmod(fd.n,remove?0644:0664)==0,
         "Cannot set the admitted torch permissions");
    return 0;
}
int command(int argc,char** argv) {
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
        if(status){std::cout<<"{\"schema\":\"org.uke.control.lights.v1\",\"physical_acceptance\":null,\"flashlight\":{\"available\":false,\"on\":null,\"brightness_percent\":null,\"requested_percent\":null,\"reason\":\"service_unavailable\"},\"rgb\":{\"available\":false,\"on\":null,\"brightness_percent\":null}}\n";return 0;}
        throw std::runtime_error(message);
    }
    const char* text=nullptr;need(sd_bus_message_read(answer,"s",&text)>0 && text,"Invalid torch service response");
    std::cout<<text<<'\n';return 0;
}
}
