// SPDX-License-Identifier: MIT
#include "backend.hpp"
#include "usb.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <thread>
#include <vector>

namespace fs = std::filesystem;
namespace uke::usb {
namespace {
std::string quote(const std::string& value) {
    constexpr char hex[]="0123456789abcdef";
    std::string out="\"";
    for(unsigned char c:value) {
        if(c=='"'||c=='\\'){out+='\\';out+=char(c);}
        else if(c<32||c>=127){out+="\\u00";out+=hex[c>>4];out+=hex[c&15];}
        else out+=char(c);
    }
    return out+'"';
}
bool beneath(const fs::path& root,const fs::path& path) {
    auto r=root.begin(),p=path.begin();
    for(;r!=root.end();++r,++p)if(p==path.end()||*r!=*p)return false;
    return true;
}
std::optional<fs::path> resolved(const fs::path& root,const fs::path& path) {
    std::error_code error;
    auto target=fs::canonical(path,error);
    if(error||!beneath(root,target))return std::nullopt;
    return target;
}
std::string attribute(const fs::path& root,const fs::path& path) {
    const auto target=resolved(root,path);
    if(!target)return "null";
    std::ifstream in(*target,std::ios::binary);
    if(!in)return "null";
    std::string result;char c;
    while(result.size()<4096&&in.get(c))result+=c;
    if(in.get(c))throw std::runtime_error("USB class attribute exceeds 4096 bytes");
    while(!result.empty()&&(result.back()=='\n'||result.back()=='\0'))result.pop_back();
    return quote(result);
}
std::vector<fs::path> entries(const fs::path& root,const fs::path& directory) {
    std::vector<fs::path> result;
    const auto target=resolved(root,directory);
    if(!target)return result;
    std::error_code error;
    for(fs::directory_iterator i(*target,error),end;!error&&i!=end;i.increment(error)) {
        if(result.size()>=256)throw std::runtime_error("USB inventory exceeds 256 class objects");
        result.push_back(i->path());
    }
    if(error)throw std::runtime_error("USB inventory directory could not be read");
    std::sort(result.begin(),result.end());return result;
}
bool exists(const fs::path& root,const fs::path& path){return bool(resolved(root,path));}
std::string driver(const fs::path& root,const fs::path& object) {
    auto target=resolved(root,object/"driver");
    if(!target)target=resolved(root,object/"device/driver");
    return target?quote(target->filename().string()):"null";
}
std::string object(const fs::path& root,const fs::path& path,const std::vector<std::string>& fields) {
    std::string result="{\"name\":"+quote(path.filename().string())+",\"driver\":"+driver(root,path);
    for(const auto& field:fields)result+=','+quote(field)+':'+attribute(root,path/field);
    return result+'}';
}
}
std::string snapshot(const fs::path& requested_root) {
    const auto root=fs::canonical(requested_root);
    const auto now=[] {return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();};
    const auto start=now();
    std::string out="{\"schema\":\"org.uke.control.usb.v1\",\"live\":"+std::string(root=="/"?"true":"false");
    out+=",\"coherence\":\"sequential_sysfs_reads\",\"power_supply_reads\":false";
    out+=",\"boot_profile\":"+attribute(root,root/"sys/firmware/devicetree/base/chosen/senemos,usb-profile");
    out+=",\"cdc_saved_policy\":"+attribute(root,root/"etc/senemos/usb-sharing");
    out+=",\"cdc_manager_state\":"+attribute(root,root/"run/senemos-uke/usb-auto-state");
    const auto group=[&](const std::string& name,const fs::path& directory,const std::vector<std::string>& fields,auto include) {
        out+=','+quote(name)+":[";bool first=true;
        for(const auto& path:entries(root,root/directory)) {
            if(!resolved(root,path)||!include(path))continue;
            if(!first)out+=',';
            first=false;out+=object(root,path,fields);
        }
        out+=']';
    };
    const auto all=[](const fs::path&){return true;};
    group("typec","sys/class/typec",{"data_role","power_role","port_type","preferred_role","orientation","power_operation_mode","supports_usb_power_delivery","usb_power_delivery_revision"},all);
    group("role_switches","sys/class/usb_role",{"role"},all);
    group("gadgets","sys/class/udc",{"state","function","current_speed","maximum_speed"},all);
    group("usb_devices","sys/bus/usb/devices",{"idVendor","idProduct","bcdDevice","bDeviceClass","busnum","devnum","devpath","speed","version","bMaxPower","authorized","configuration"},[&](const fs::path& p){return exists(root,p/"idVendor");});
    group("usb_interfaces","sys/bus/usb/devices",{"bInterfaceClass","bInterfaceSubClass","bInterfaceProtocol"},[&](const fs::path& p){return exists(root,p/"bInterfaceClass");});
    group("usb_network","sys/class/net",{"operstate","carrier","speed","duplex","statistics/rx_bytes","statistics/tx_bytes","statistics/rx_errors","statistics/tx_errors","statistics/rx_dropped","statistics/tx_dropped"},[&](const fs::path& p){return exists(root,p/"device/bInterfaceClass");});
    group("displays","sys/class/drm",{"status","enabled","modes"},[](const fs::path& p){return p.filename().string().find("-DP-")!=std::string::npos||p.filename().string().find("-HDMI-A-")!=std::string::npos;});
    out+=",\"started_monotonic_ms\":"+std::to_string(start)+",\"finished_monotonic_ms\":"+std::to_string(now())+'}';
    if(out.size()>1024*1024)throw std::runtime_error("USB snapshot exceeds 1 MiB");
    return out+'\n';
}
}
namespace uke {
std::string usb_status(){return usb::snapshot("/");}
void usb_watch(unsigned seconds) {
    if(seconds<1||seconds>300)throw std::runtime_error("USB watch duration must be 1..300 seconds");
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(seconds);
    do {
        std::cout<<usb_status()<<std::flush;
        const auto next=std::chrono::steady_clock::now()+std::chrono::seconds(1);
        std::this_thread::sleep_until(std::min(next,deadline));
    } while(std::chrono::steady_clock::now()<deadline);
}
}
