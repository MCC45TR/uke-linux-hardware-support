// SPDX-License-Identifier: MIT
// One-shot user configuration transition. No sensor reads or idle process.
#include "brightness-curve-config.hpp"
#include <systemd/sd-bus.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <optional>
#include <pwd.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

namespace fs=std::filesystem;
namespace {
using namespace uke::brightness;
struct Fd {
    int n=-1;
    explicit Fd(int value=-1):n(value){}
    ~Fd(){if(n>=0)close(n);}
    Fd(const Fd&)=delete;
};
std::optional<std::string> read_file(const fs::path& path,bool optional=false) {
    Fd file(open(path.c_str(),O_RDONLY|O_NOFOLLOW|O_CLOEXEC|O_NONBLOCK));
    if(file.n<0&&errno==ENOENT&&optional)return std::nullopt;
    struct stat st{};
    require(file.n>=0&&!fstat(file.n,&st)&&S_ISREG(st.st_mode)&&st.st_uid==getuid()&&
        st.st_nlink==1&&!(st.st_mode&0022)&&st.st_size>=0&&st.st_size<=1024*1024,
        "Untrusted or unavailable brightness configuration file");
    std::string data;char buffer[4096];
    for(;;){const auto count=read(file.n,buffer,sizeof(buffer));if(count<0&&errno==EINTR)continue;
        require(count>=0,"Cannot read brightness configuration");if(!count)break;
        data.append(buffer,size_t(count));require(data.size()<=1024*1024,"Brightness file exceeds bounds");}
    return data;
}
void directory(const fs::path& path,bool create) {
    if(create) {
        std::error_code error;fs::create_directories(path,error);
        require(!error,"Cannot create brightness settings directory");
    }
    struct stat st{};
    require(!lstat(path.c_str(),&st)&&S_ISDIR(st.st_mode)&&st.st_uid==getuid()&&!(st.st_mode&0022),
        "Untrusted brightness settings directory");
}
void publish(const fs::path& path,const std::string& data) {
    require(data.size()<=1024*1024,"Brightness publication exceeds bounds");
    directory(path.parent_path(),false);
    std::string name=(path.parent_path()/".uke-brightness-XXXXXX").string();
    std::vector<char> temp(name.begin(),name.end());temp.push_back('\0');
    Fd file(mkstemp(temp.data()));require(file.n>=0,"Cannot prepare brightness settings");
    try {
        size_t offset=0;
        while(offset<data.size()) {
            const auto count=write(file.n,data.data()+offset,data.size()-offset);
            if(count<0&&errno==EINTR)continue;
            require(count>0,"Cannot write brightness settings");offset+=size_t(count);
        }
        require(!fsync(file.n),"Cannot sync brightness settings");
        require(!rename(temp.data(),path.c_str()),"Cannot publish brightness settings");
        Fd parent(open(path.parent_path().c_str(),O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC));
        require(parent.n>=0&&!fsync(parent.n),"Cannot sync brightness settings directory");
    }catch(...){unlink(temp.data());throw;}
}
bool owned_name(sd_bus* bus,const char* service) {
    sd_bus_message* reply=nullptr;
    const int result=sd_bus_call_method(bus,"org.freedesktop.DBus","/org/freedesktop/DBus",
        "org.freedesktop.DBus","NameHasOwner",nullptr,&reply,"s",service);
    const std::unique_ptr<sd_bus_message,decltype(&sd_bus_message_unref)> message(reply,sd_bus_message_unref);
    int owned=0;
    require(result>=0&&sd_bus_message_read(reply,"b",&owned)>0,"Cannot verify compositor absence");
    return owned;
}
void before_compositor() {
    sd_bus* raw=nullptr;
    require(sd_bus_open_user(&raw)>=0,"User bus unavailable; pending brightness curve preserved");
    const std::unique_ptr<sd_bus,decltype(&sd_bus_unref)> bus(raw,sd_bus_unref);
    require(sd_bus_set_method_call_timeout(raw,1000000)>=0,"Cannot bound compositor query");
    require(!owned_name(raw,"org.kde.KWin")&&!owned_name(raw,"org.gnome.Mutter.DisplayConfig"),
        "Apply brightness curves only before a fresh compositor starts");
}
void native_normal_panel() {
    std::ifstream input("/proc/device-tree/compatible",std::ios::binary);
    std::string bytes((std::istreambuf_iterator<char>(input)),{});
    require(input.is_open()&&!input.bad()&&bytes.size()<=4096,"Cannot identify the Uke board");
    bool uke=false;
    for(size_t start=0;start<bytes.size();) {
        const auto end=bytes.find('\0',start);require(end!=std::string::npos,"Malformed board identity");
        uke|=bytes.substr(start,end-start)=="xiaomi,uke";start=end+1;
    }
    require(uke,"OSS curve requires the exact Uke board");
    unsigned backlights=0;
    for(const auto& entry:fs::directory_iterator("/sys/class/backlight")) {
        std::ifstream value(entry.path()/"max_brightness");std::string text,extra;
        require(bool(std::getline(value,text))&&!std::getline(value,extra),"Cannot inspect native brightness range");
        if(text=="2047")++backlights;
    }
    require(backlights==1,"Unique native normal backlight range unavailable");
}
int command(const std::string& operation) {
    require(getuid()!=0&&getuid()==geteuid(),"Run brightness preferences as the desktop user");
    const passwd* account=getpwuid(getuid());require(account&&account->pw_dir,"Unknown desktop user");
    const char* setting=std::getenv("XDG_CONFIG_HOME");
    const fs::path config=setting&&*setting?fs::path(setting):fs::path(account->pw_dir)/".config";
    require(config.is_absolute()&&config.lexically_normal()==config,"Invalid user configuration directory");
    const auto state=config/"uke-control",pending=state/"brightness-curve-request.json";
    const auto saved=state/"brightness-curve-applied.json",target=config/"kwinoutputconfig.json";
    // No work when no request exists: the optional startup hook is one-shot.
    if(operation=="apply-pending"&&!fs::exists(pending))return 0;
    directory(config,false);
    if(operation=="status") {
        const auto current=read_file(target);
        auto document=parse(*current);auto& output=panel(document);
        const bool has=output.isMember("autoBrightnessCurve");
        if(has)validate_curve(output["autoBrightnessCurve"]);
        std::cout<<"{\"schema\":\"org.uke.control.brightness-curve.v1\",\"configured_points\":"
            <<(has?output["autoBrightnessCurve"].size():0)<<",\"pending\":"
            <<(fs::exists(pending)?"true":"false")<<",\"saved_restore\":"
            <<(fs::exists(saved)?"true":"false")<<",\"active_curve\":null,\"physical_acceptance\":null}\n";
        return 0;
    }
    require(operation=="queue-oss"||operation=="queue-restore"||operation=="cancel"||operation=="apply-pending",
        "Usage: uke-control desktop brightness-curve status|queue-oss|queue-restore|cancel");
    directory(state,true);
    Fd lock(open((state/"brightness-curve.lock").c_str(),O_CREAT|O_RDWR|O_NOFOLLOW|O_CLOEXEC,0600));
    struct stat st{};
    require(lock.n>=0&&!fstat(lock.n,&st)&&S_ISREG(st.st_mode)&&st.st_uid==getuid()&&st.st_nlink==1&&
        (st.st_mode&0777)==0600&&!flock(lock.n,LOCK_EX|LOCK_NB),"Brightness transition is busy or untrusted");
    if(operation=="cancel") {
        const auto queued=read_file(pending,true);
        if(queued) {
            const auto request=parse(*queued);
            const auto current=read_file(target);
            if(request["operation"]=="oss-normal"&&request["original_config"].isString()&&
               request["original_config"].asString()==*current) {
                const auto intended=apply_request(*current,request);
                const auto old=read_file(saved,true);
                if(old&&*old==serialize(intended.applied))
                    require(!unlink(saved.c_str()),"Cannot cancel unused restore data");
            }
        }
        require(!unlink(pending.c_str())||errno==ENOENT,"Cannot cancel pending brightness request");
        std::cout<<"Pending brightness request canceled; current settings preserved.\n";return 0;
    }
    native_normal_panel();
    const auto current=read_file(target);
    if(operation!="apply-pending") {
        require(!read_file(pending,true),"A brightness transition is already queued");
        const auto previous=read_file(saved,true);
        require(operation!="queue-oss"||!previous,"Restore the saved curve before another OSS import");
        const auto request=make_request(*current,operation=="queue-restore",previous?parse(*previous):Json::Value{});
        publish(pending,serialize(request));
        std::cout<<"Brightness curve queued for the next fresh Plasma session; current session unchanged.\n";return 0;
    }
    before_compositor();
    const auto request=parse(*read_file(pending));
    require(request["original_config"].isString(),"Invalid pending brightness request");
    const auto transition=apply_request(request["original_config"].asString(),request);
    const bool committed=*current==transition.configuration;
    require(committed||*current==request["original_config"].asString(),
        "Brightness request is stale; current preferences preserved");
    const bool restoring=request["operation"]=="restore";
    // Save only the curve, not an obsolete copy of external display layouts.
    // A failed target publication leaves the request and restore data intact.
    if(!restoring) {
        const auto old=read_file(saved,true);
        const auto receipt=serialize(transition.applied);
        require(!old||*old==receipt,"Existing brightness restore data differs");
        if(!old)publish(saved,receipt);
    }
    require(*read_file(target)==*current,"KWin settings changed during preparation");
    before_compositor();
    if(!committed)publish(target,transition.configuration);
    if(restoring)require(!unlink(saved.c_str())||errno==ENOENT,"Curve restored; cannot remove old restore record");
    require(!unlink(pending.c_str()),"Curve applied; cannot remove old request");
    std::cout<<"Internal brightness curve "<<(restoring?"restored":"imported")
        <<"; other output preferences preserved. Active response requires session and physical validation.\n";
    return 0;
}
}
#ifndef UKE_BRIGHTNESS_CURVE_TEST
int main(int argc,char** argv) {
    try {
        uke::brightness::require(argc==2,"Usage: brightness-curve OPERATION");
        return command(argv[1]);
    }catch(const std::exception& error){std::cerr<<"uke-control brightness curve: "<<error.what()<<'\n';return 1;}
}
#endif
