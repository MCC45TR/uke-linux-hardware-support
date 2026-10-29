// SPDX-License-Identifier: MIT
#include "lights.hpp"
#include "rgb.hpp"
#include "backend.hpp"
#include "sysfs.hpp"
#include "wake.hpp"
#include <algorithm>
#include <charconv>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <fcntl.h>
#include <iostream>
#include <linux/magic.h>
#include <poll.h>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <systemd/sd-daemon.h>
#include <thread>
#include <unistd.h>
namespace fs=std::filesystem;
namespace uke::lights {
static constexpr const char* pmic="soc/qcom,spmi@c42d000/qcom,pmxr2230@1";
static constexpr const char* unit="uke-control-flashlight.service";
static void need(bool ok, const std::string& message) { if(!ok)throw std::runtime_error(message); }
struct Fd { int n=-1; ~Fd(){if(n>=0)close(n);} };
static std::string raw(const fs::path& sys, const fs::path& path) {
    auto target=sysfs::resolved(sys,path);
    need(bool(target),"Light attribute unavailable");
    Fd fd{open(target->c_str(),O_RDONLY|O_NONBLOCK|O_NOFOLLOW|O_CLOEXEC)};
    struct stat info{};
    need(fd.n>=0 && !fstat(fd.n,&info) && S_ISREG(info.st_mode),"Invalid light attribute");
    std::string out; char buffer[4097];
    const auto n=::read(fd.n,buffer,sizeof(buffer));
    need(n>=0 && n<=4096,"Unreadable or oversized light attribute");
    out.assign(buffer,size_t(n));
    while(!out.empty() && out.back()=='\n')out.pop_back();
    return out;
}
static unsigned number(const std::string& value, unsigned maximum) {
    unsigned n=0;
    auto [end,error]=std::from_chars(value.data(),value.data()+value.size(),n);
    need(!value.empty() && error==std::errc{} && end==value.data()+value.size() && n<=maximum,"Light value out of range");
    return n;
}
static std::string cell(unsigned n) {
    std::string value(4,'\0');
    for(unsigned i=0;i<4;++i)value[i]=char(n>>(24-8*i));
    return value;
}
static bool compatible(const fs::path& sys,const fs::path& node,const std::string& expected) {
    const auto value=raw(sys,node/"compatible");
    for(size_t start=0;start<value.size();) {
        auto end=value.find('\0',start);
        if(end==value.npos)return false;
        if(value.substr(start,end-start)==expected)return true;
        start=end+1;
    }
    return false;
}
unsigned color(const std::string& value) {return number(value,255);}
unsigned percent(const std::string& value) {return number(value,100);}
unsigned level(unsigned value,unsigned maximum) {
    need(value<=100 && maximum>0,"Invalid light percentage or scale");
    return value?std::max(1U,value*maximum/100):0;
}
std::string intensities(const std::string& order,const std::array<unsigned,3>& rgb) {
    for(auto value:rgb)need(value<=255,"RGB component exceeds 255");
    std::istringstream in(order); std::string name,out; unsigned mask=0;
    while(in>>name) {
        unsigned index=name=="red"?0:name=="green"?1:name=="blue"?2:3;
        need(index<3 && !(mask&(1U<<index)),"RGB channel order is unavailable or ambiguous");
        mask|=1U<<index;
        if(!out.empty())out+=' ';
        out+=std::to_string(rgb[index]*511/255);
    }
    need(mask==7,"RGB channel order is incomplete");
    return out;
}
std::array<unsigned,3> colors(const std::string& order,const std::string& values) {
    (void)intensities(order,{0,0,0});
    std::istringstream names(order),in(values);std::string name,value;
    std::array<unsigned,3> result{};
    while(names>>name) {
        need(bool(in>>value),"RGB intensities are incomplete");
        const unsigned index=name=="red"?0:name=="green"?1:2;
        result[index]=(number(value,511)*255+255)/511;
    }
    need(!(in>>value),"RGB intensities are ambiguous");
    return result;
}
Source discover(bool rgb,const fs::path& sys) {
    Source out;
    try {
        need(wake::applicable(sys),"uke_identity_unavailable");
        const auto name=rgb?"uke:rgb:status":"uke:white:torch";
        const auto path=sys/"class/leds"/name;
        const auto target=sysfs::resolved(sys/"devices",path);
        need(bool(target),"matched_kernel_light_provider_unavailable");
        const auto base=sys/"firmware/devicetree/base"/pmic;
        const auto controller=base/(rgb?"pwm":"led-controller@ee00");
        const auto node=controller/(rgb?"multi-led":"led-0");
        const auto actual=sysfs::resolved(sys,path/"device/of_node");
        need(actual && *actual==controller,"light_dt_identity_mismatch");
        need(raw(sys,node/"label")==std::string(name)+'\0',"light_dt_label_mismatch");
        for(const auto& child:sysfs::entries(sys,controller))
            need(!fs::is_directory(child) || child==node,"ambiguous_light_dt_children");
        need(sysfs::driver(sys,path)==sysfs::quote(rgb?"qcom-spmi-lpg":"leds-qcom-flash"),"light_driver_mismatch");
        need(compatible(sys,controller,rgb?"qcom,pm8350c-pwm":"qcom,spmi-flash-led"),"light_controller_mismatch");
        need(raw(sys,path/"max_brightness")== (rgb?"511":"2"),"light_scale_mismatch");
        const auto trigger=raw(sys,path/"trigger");
        need(trigger.find("[none]")!=std::string::npos,"light_owned_by_another_trigger");
        if(rgb) {
            for(unsigned i=1;i<=3;++i) {
                auto child=node/("led@"+std::to_string(i));
                need(raw(sys,child/"reg")==cell(i) && raw(sys,child/"color")==cell(i),"rgb_channel_mapping_mismatch");
            }
            out.order=raw(sys,path/"multi_index");
            out.rgb=colors(out.order,raw(sys,path/"multi_intensity"));
        } else {
            need(raw(sys,controller/"reg")==cell(0xee00),"torch_register_mapping_mismatch");
            need(raw(sys,node/"led-sources")==cell(1)+cell(4),"torch_channel_mapping_mismatch");
            need(raw(sys,node/"led-max-microamp")==cell(20000),"torch_current_limit_mismatch");
            need(!sysfs::exists(sys,node/"flash-max-microamp") && !sysfs::exists(sys,path/"flash_strobe"),"flash_mode_not_admitted");
        }
        out.brightness=number(raw(sys,path/"brightness"),rgb?511:2);
        out.path=*target;
    } catch(const std::exception& e) {out.reason=e.what();out.path.clear();}
    return out;
}
std::string snapshot(const fs::path& sys) {
    std::string out="{\"schema\":\"org.uke.control.lights.v1\",\"physical_acceptance\":null";
    for(bool rgb:{false,true}) {
        const auto source=discover(rgb,sys); const bool ok=!source.path.empty();
        out+=std::string(",\"")+(rgb?"rgb":"flashlight")+"\":{\"available\":"+(ok?"true":"false")+
            ",\"on\":"+(ok?(source.brightness?"true":"false"):"null")+
            ",\"brightness_percent\":"+(ok?std::to_string(rgb?std::min(100U,(source.brightness*100+4)/8):source.brightness*50):"null")+
            ",\"reason\":"+(ok?"null":sysfs::quote(source.reason))+
            (rgb?",\"color\":"+(ok?"["+std::to_string(source.rgb[0])+","+std::to_string(source.rgb[1])+","+std::to_string(source.rgb[2])+"]":"null")+
                 ",\"pwm_brightness_limit\":8,\"pwm_max\":511":",\"max_microamp\":20000,\"brightness_step_percent\":50,\"max_seconds\":null")+"}";
    }
    return out+"}\n";
}
static void root() {need(geteuid()==0,"Use sudo uke-control for light changes");need(wake::applicable(),"Light changes require exact Uke / SM7675 identity");}
static Source required(bool rgb) {auto s=discover(rgb);need(!s.path.empty(),s.reason);return s;}
static void write_fd(int fd,const std::string& value) {
    struct statfs type{};
    need(fd>=0 && !fstatfs(fd,&type) && type.f_type==SYSFS_MAGIC,"Light target is not sysfs");
    need(lseek(fd,0,SEEK_SET)==0,"Cannot seek light attribute");
    ssize_t n;do{n=write(fd,value.data(),value.size());}while(n<0 && errno==EINTR);
    need(n==ssize_t(value.size()),"Light write failed");
}
static void put(const fs::path& path,const std::string& value) {
    Fd fd{open(path.c_str(),O_WRONLY|O_NOFOLLOW|O_CLOEXEC)};
    write_fd(fd.n,value+"\n");
    need(raw("/sys",path)==value,"Light readback differs");
}
struct Lock {
    Fd fd;
    explicit Lock(bool rgb,bool wait=false):fd{open(rgb?"/run/uke-control-rgb.lock":"/run/uke-control-torch.lock",O_CREAT|O_RDWR|O_NOFOLLOW|O_CLOEXEC,0600)} {
        struct stat info{};
        need(fd.n>=0 && !fstat(fd.n,&info) && S_ISREG(info.st_mode) && info.st_uid==0 && info.st_nlink==1 && !(info.st_mode&0077),"Invalid light lock");
        need(flock(fd.n,LOCK_EX|(wait?0:LOCK_NB))==0,"Another light operation is active");
    }
};
static void save_target(unsigned value) {
    need(value<=2,"Invalid torch target");Lock lock(false);
    const char byte=char('0'+value);
    need(pwrite(lock.fd.n,&byte,1,0)==1 && ftruncate(lock.fd.n,1)==0,"Cannot save torch target");
}
static unsigned read_target() {
    Lock lock(false,true);char bytes[2];
    need(pread(lock.fd.n,bytes,2,0)==1 && bytes[0]>='0' && bytes[0]<='2',"Invalid torch target");
    return unsigned(bytes[0]-'0');
}
static volatile sig_atomic_t stopped=0;
static void stop(int) {stopped=1;}
static void hold() {
    root(); const auto source=required(false);
    need(source.brightness==0,"Torch already owned outside the managed service");
    for(int sig:{SIGTERM,SIGINT,SIGHUP})std::signal(sig,stop);
    Fd fd{open((source.path/"brightness").c_str(),O_WRONLY|O_NOFOLLOW|O_CLOEXEC)};
    struct Off {int fd;~Off(){try{write_fd(fd,"0\n");}catch(const std::exception& e){std::cerr<<e.what()<<'\n';}}} off{fd.n};
    bool ready=false;unsigned previous=3;
    while(!stopped) {
        const unsigned desired=read_target();
        if(stopped)break;
        if(desired!=previous) {
            write_fd(fd.n,std::to_string(desired)+"\n");
            need(raw("/sys",source.path/"brightness")==std::to_string(desired),"Torch brightness readback differs");
            previous=desired;
        }
        if(!ready) {need(sd_notify(0,"READY=1")>=0,"Cannot notify the torch supervisor");ready=true;}
        if(!desired)break;
        (void)poll(nullptr,0,100);
    }
}
int command(bool rgb,int argc,char** argv) {
    if(rgb)return uke::rgb::command(argc,argv);
    if(argc==1 && std::string_view(argv[0])=="status") {std::cout<<snapshot();return 0;}
    if(!rgb && argc==1) {
        const std::string action=argv[0];
        if(action=="hold") {hold();return 0;}
        if(action=="reset") {root();Lock lock(false);put(required(false).path/"brightness","0");return 0;}
    }
    if(!rgb && ((argc==1 && (std::string_view(argv[0])=="on" || std::string_view(argv[0])=="off")) ||
                (argc==2 && std::string_view(argv[0])=="set"))) {
        const unsigned desired=argc==2?level(percent(argv[1]),2):(std::string_view(argv[0])=="on"?2:0);
        root();if(desired){(void)required(false);save_target(desired);}
        const auto result=execute({"/usr/bin/systemctl",desired?"start":"stop",unit},10,8192);
        need(result.status==0,"Torch supervisor request failed: "+result.output);
        if(!desired) {save_target(0);Lock lock(false);put(required(false).path/"brightness","0");}
        bool applied=false;
        for(unsigned i=0;i<30;++i) {
            if(required(false).brightness==desired){applied=true;break;}
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        need(applied,"Torch brightness was not applied");
        std::cout<<snapshot();return 0;
    }
    throw std::runtime_error("Usage: uke-control flashlight status|on|off|set BRIGHTNESS (0..100; hardware steps 0, 50, 100)");
}
}
