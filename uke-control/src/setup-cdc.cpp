// SPDX-License-Identifier: MIT
// Temporary first-setup transport in the existing Control executable.
#include "setup-cdc.hpp"
#include "backend.hpp"
#include "sysfs.hpp"
#include <array>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <fcntl.h>
#include <poll.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <sys/sysmacros.h>
#include <termios.h>
#include <unistd.h>

namespace uke::setup_cdc {
namespace fs=std::filesystem;
namespace {
constexpr const char* marker="/etc/uke-control/first-setup-cdc";
constexpr const char* done="/etc/plasma-setup-done";
constexpr const char* runtime="/run/uke-control-setup";
struct Fd {
    int n=-1;
    explicit Fd(int value):n(value){}
    Fd(const Fd&)=delete;
    Fd& operator=(const Fd&)=delete;
    ~Fd(){if(n>=0)close(n);}
};
void need(bool value,const char* why){if(!value)throw std::runtime_error(why);}
bool present(const char* path) {
    struct stat st{};
    if(lstat(path,&st)==0)return true;
    need(errno==ENOENT,"Cannot inspect setup state");return false;
}
bool complete(){return present(done);} // Any existing flag closes access.
void directory(int fd) {
    struct stat st{};
    need(fd>=0&&!fstat(fd,&st)&&S_ISDIR(st.st_mode)&&st.st_uid==0&&!(st.st_mode&0022),"Untrusted setup state directory");
}
bool exact_file(const char* path,const char* value,bool optional) {
    Fd fd(open(path,O_RDONLY|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC));
    if(fd.n<0&&errno==ENOENT&&optional)return false;
    struct stat st{};const auto bytes=std::strlen(value);
    need(fd.n>=0&&!fstat(fd.n,&st)&&S_ISREG(st.st_mode)&&st.st_uid==0&&
        st.st_nlink==1&&!(st.st_mode&0022)&&st.st_size==off_t(bytes),"Untrusted setup state file");
    std::array<char,32> buffer{};
    need(bytes<buffer.size()&&::read(fd.n,buffer.data(),buffer.size())==ssize_t(bytes)&&
        std::string_view(buffer.data(),bytes)==value,"Invalid setup state contents");
    return true;
}
void guard() {
    need(geteuid()==0,"Setup CDC requires root");
    const auto board=uke::read("/sys/firmware/devicetree/base/compatible",4096);
    const auto token=[&](const std::string& name) {
        const auto at=board.find(name+'\0');
        return at!=board.npos&&(!at||board[at-1]=='\0');
    };
    need(token("xiaomi,uke")&&token("qcom,sm7675"),"Setup CDC requires Uke/SM7675");
    Fd parent(open("/etc/uke-control",O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC));
    directory(parent.n);exact_file(marker,"v1\n",false);
}
struct PolicyLock {
    Fd parent{open("/var/lib/uke-control",O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC)};
    Fd lock{-1};
    PolicyLock() {
        directory(parent.n);
        lock.n=openat(parent.n,"cdc-policy.lock",O_CREAT|O_RDWR|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC,0600);
        struct stat st{};
        need(lock.n>=0&&!fstat(lock.n,&st)&&S_ISREG(st.st_mode)&&st.st_uid==0&&st.st_nlink==1&&
            (st.st_mode&0777)==0600&&flock(lock.n,LOCK_EX)==0,"Cannot lock CDC policy");
    }
};
bool saved() {
    const char* path="/var/lib/uke-control/cdc-on";
    const bool enabled=exact_file(path,"on\n",true);
    if(enabled) {
        struct stat st{};need(!lstat(path,&st)&&!(st.st_mode&0077),"Untrusted saved CDC mode");
    }
    return enabled;
}
bool claimed(){
    Fd parent(open(runtime,O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC));
    if(parent.n<0&&errno==ENOENT)return false;
    directory(parent.n);
    return exact_file("/run/uke-control-setup/cdc-owned","v1\n",true);
}
bool ready() {
    if(complete())return false;
    Fd parent(open("/run/senemos-uke",O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC));
    if(parent.n<0&&errno==ENOENT)return false;
    directory(parent.n);
    if(!exact_file("/run/senemos-uke/usb-gadget-profile","manual-acm\n",true))return false;
    const auto udc=uke::read("/sys/kernel/config/usb_gadget/uke_debug/UDC",128);
    return !udc.empty()&&udc!="\n"&&udc!="unavailable\n";
}
void claim() {
    if(mkdir(runtime,0700)<0)need(errno==EEXIST,"Cannot create setup runtime");
    Fd parent(open(runtime,O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC));directory(parent.n);
    Fd fd(openat(parent.n,"cdc-owned",O_CREAT|O_EXCL|O_WRONLY|O_NOFOLLOW|O_CLOEXEC,0600));
    if(fd.n<0&&errno==EEXIST){need(claimed(),"Invalid setup claim");return;}
    need(fd.n>=0,"Cannot create temporary CDC ownership");
    size_t stored=0;
    while(stored<3) {
        const auto count=write(fd.n,"v1\n"+stored,3-stored);
        if(count<0&&errno==EINTR)continue;
        if(count<=0){unlinkat(parent.n,"cdc-owned",0);throw std::runtime_error("Cannot record temporary CDC ownership");}
        stored+=size_t(count);
    }
}
int finish() {
    need(complete(),"Plasma setup has not completed");
    const auto stopped=uke::execute_bounded({"/usr/bin/systemctl","stop","uke-control-setup-console.service"},20,4096);
    if(stopped.status)return stopped.status;
    {
        PolicyLock lock;
        if(claimed()) {
            if(!saved()) {
                const auto result=uke::cdc_set(false);
                if(result.status){std::cerr<<result.output;return result.status;}
            }
            need(unlink("/run/uke-control-setup/cdc-owned")==0,"Cannot release setup CDC claim");
        }
    }
    const auto disabled=uke::execute_bounded({"/usr/bin/systemctl","disable","uke-control-setup-cdc.service","uke-control-setup-cdc-finish.path"},20,4096);
    if(disabled.status)return disabled.status;
    const auto ended=uke::execute_bounded({"/usr/bin/systemctl","--no-block","stop","uke-control-setup-cdc.service","uke-control-setup-cdc-finish.path"},5,4096);
    if(ended.status)return ended.status;
    std::cout<<"{\"setup_cdc\":\"finished\",\"saved_policy_changed\":false}\n";
    return 0;
}
int apply() {
    if(complete())return finish();
    PolicyLock lock;
    if(!saved())claim();
    const auto result=uke::cdc_set(true);
    if(result.status==69) {
        std::cout<<"{\"setup_cdc\":\"waiting_for_device_role\",\"saved_policy_changed\":false}\n";
        return 0; // A later Type-C/UDC event retries; no polling worker.
    }
    std::cout<<result.output;return result.status;
}
void response(int fd,const std::string& value) {
    const auto message=value+'\n';size_t sent=0;
    const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(5);
    while(sent<message.size()) {
        const auto remaining=std::chrono::duration_cast<std::chrono::milliseconds>(end-std::chrono::steady_clock::now()).count();
        need(remaining>0,"Setup host stopped reading");
        pollfd wait{fd,POLLOUT,0};const auto result=poll(&wait,1,int(remaining));
        if(result<0&&errno==EINTR)continue;
        need(result>0&&!(wait.revents&(POLLHUP|POLLERR|POLLNVAL)),"Setup USB write disconnected");
        const auto count=write(fd,message.data()+sent,message.size()-sent);
        if(count<0&&(errno==EINTR||errno==EAGAIN))continue;
        need(count>0,"Cannot write setup response");sent+=size_t(count);
    }
}
void wait_configured(int fd) {
    for(;;) {
        std::array<char,64> state{};
        need(lseek(fd,0,SEEK_SET)==0,"Cannot seek UDC state");
        const auto bytes=::read(fd,state.data(),state.size());
        if(bytes<0&&errno==EINTR)continue;
        need(bytes>0&&size_t(bytes)<state.size(),"Cannot read UDC state");
        if(std::string_view(state.data(),size_t(bytes))=="configured\n")return;
        // usb_gadget_set_state() notifies this sysfs attribute. No idle timer.
        pollfd changed{fd,POLLPRI,0};
        int result;
        do{result=poll(&changed,1,-1);}while(result<0&&errno==EINTR);
        need(result>0&&!(changed.revents&POLLNVAL),"Cannot wait for USB configuration");
    }
}
int console() {
    need(ready(),"The first-setup CDC endpoint is not owned and ready");
    auto controller=uke::read("/sys/kernel/config/usb_gadget/uke_debug/UDC",128);
    need(controller.ends_with('\n'),"Invalid setup controller");controller.pop_back();
    need(!controller.empty()&&controller!="."&&controller!=".."&&controller.size()<64&&controller.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.:-")==controller.npos,"Invalid setup controller name");
    Fd state(open(("/sys/class/udc/"+controller+"/state").c_str(),O_RDONLY|O_CLOEXEC));
    need(state.n>=0,"Cannot open setup UDC state");wait_configured(state.n);
    need(ready(),"Setup CDC ownership changed");
    Fd terminal(open("/dev/ttyGS0",O_RDWR|O_NOCTTY|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC));
    struct stat st{};
    need(terminal.n>=0&&!fstat(terminal.n,&st)&&S_ISCHR(st.st_mode)&&st.st_uid==0&&st.st_nlink==1&&!(st.st_mode&0002)&&isatty(terminal.n),"Untrusted setup CDC terminal");
    const auto identity=uke::read("/sys/class/tty/ttyGS0/dev",64);
    const auto colon=identity.find(':');unsigned major_number=0,minor_number=0;
    need(colon!=identity.npos&&identity.ends_with('\n'),"Invalid setup terminal identity");
    const auto first=std::from_chars(identity.data(),identity.data()+colon,major_number);
    const auto second=std::from_chars(identity.data()+colon+1,identity.data()+identity.size()-1,minor_number);
    need(first.ec==std::errc{}&&first.ptr==identity.data()+colon&&second.ec==std::errc{}&&second.ptr==identity.data()+identity.size()-1&&st.st_rdev==makedev(major_number,minor_number),"Setup terminal differs from its kernel device");
    termios mode{};need(!tcgetattr(terminal.n,&mode),"Cannot inspect setup terminal");
    cfmakeraw(&mode);need(!tcsetattr(terminal.n,TCSANOW,&mode),"Cannot configure setup terminal");
    const auto send=[&](const std::string& message){response(terminal.n,message);};
    send("{\"schema\":\"org.uke.setup-cdc.v1\",\"commands\":[\"help\",\"status\",\"logs\",\"reboot bootloader\"],\"root_shell\":false}");
    std::string line;line.reserve(64);char byte=0;
    for(;;) {
        pollfd input{terminal.n,POLLIN,0};
        const auto result=poll(&input,1,-1);
        if(result<0&&errno==EINTR)continue;
        need(result>0,"Cannot wait for setup input");
        if(input.revents&(POLLHUP|POLLERR|POLLNVAL))return 0;
        const auto received=::read(terminal.n,&byte,1);
        if(received<0&&errno==EAGAIN)continue;
        if(received<0&&errno==EINTR)continue;
        if(received<=0)return 0;
        if(byte!='\r'&&byte!='\n') {
            if(byte<32||byte>126||line.size()==63)return 65;
            line+=byte;continue;
        }
        if(line.empty())continue;
        need(ready(),"First-setup CDC ownership or completion changed");
        switch(request(line)) {
        case Request::help:
            send("{\"commands\":[\"help\",\"status\",\"logs\",\"reboot bootloader\"]}");break;
        case Request::status: {
            utsname name{};need(!uname(&name),"Cannot read kernel identity");
            send("{\"setup_active\":true,\"kernel\":"+sysfs::quote(name.release)+"}");break;
        }
        case Request::logs: {
            const auto result=uke::execute_bounded({"/usr/bin/journalctl","-b","-k","-n","128","--no-pager","-o","short-monotonic"},5,16384);
            send("{\"status\":"+std::to_string(result.status)+",\"truncated\":"+(result.truncated?"true":"false")+",\"kernel_log\":"+sysfs::quote(result.output)+"}");break;
        }
        case Request::bootloader: {
            send("{\"requested\":\"reboot bootloader\"}");
            const auto result=uke::execute_bounded({"/usr/bin/uke-reboot","bootloader"},8,4096);
            if(result.status)send("{\"status\":"+std::to_string(result.status)+",\"error\":"+sysfs::quote(result.output)+"}");
            return result.status;
        }
        case Request::invalid:send("{\"error\":\"unknown_command\"}");break;
        }
        line.clear();
    }
}
}
int command(int argc,char** argv) {
    need(argc==1,"Expected setup-cdc apply, finish, ready or console");
    const std::string_view mode=argv[0];
    need(mode=="apply"||mode=="finish"||mode=="ready"||mode=="console","Unknown setup CDC operation");
    guard();
    if(mode=="apply")return apply();
    if(mode=="finish")return finish();
    if(mode=="ready")return ready()?0:1;
    if(mode=="console")return console();
    throw std::runtime_error("Unknown setup CDC operation");
}
}
