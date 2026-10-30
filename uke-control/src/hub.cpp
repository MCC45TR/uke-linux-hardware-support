// SPDX-License-Identifier: MIT
#include "hub.hpp"
#include "sysfs.hpp"
#include <charconv>
#include <sstream>
namespace uke::hub {
namespace {
using namespace uke::sysfs;
struct Value {std::string state,text;};
Value text(const fs::path& root,const fs::path& path) {
    std::error_code error;
    const auto status=fs::symlink_status(path,error);
    if(error==std::errc::no_such_file_or_directory || status.type()==fs::file_type::not_found)return {"unknown","not exported"};
    const auto target=resolved(root,path);if(error||!target)return {"error","unreadable or uncontained interface"};
    std::ifstream in(*target);if(!in)return {"error","permission or read failure"};
    std::string value;char c;while(value.size()<4096&&in.get(c))value+=c;
    if(in.get(c)||in.bad())return {"error","invalid interface length/read"};
    while(!value.empty()&&value.back()=='\n')value.pop_back();
    if(value.empty())return {"unknown","no sample"};
    return {"available",value};
}
Value integer(const fs::path& root,const fs::path& path,long long minimum,long long maximum) {
    auto result=text(root,path);if(result.state!="available")return result;
    long long n{};const auto [end,error]=std::from_chars(result.text.data(),result.text.data()+result.text.size(),n);
    if(error!=std::errc{}||end!=result.text.data()+result.text.size()||n<minimum||n>maximum)return {"error","invalid numeric sample"};
    result.text=std::to_string(n);return result;
}
std::string item(const std::string& id,const std::string& label,const Value& value,const std::string& unit={}) {
    return "{\"id\":"+quote(id)+",\"label\":"+quote(label)+",\"state\":"+quote(value.state)+",\"value\":"+(value.state=="available"?quote(value.text):"null")+",\"unit\":"+quote(unit)+",\"reason\":"+(value.state=="available"?"null":quote(value.text))+"}";
}
}
std::string snapshot(const fs::path& given) {
    const auto root=fs::canonical(given);
    std::string out="{\"schema\":\"org.uke.control.hub.v1\",\"live\":"+std::string(root=="/"?"true":"false")+",\"read_only\":true,\"physical_acceptance\":null,\"state_semantics\":\"kernel_interface_observation_not_physical_acceptance\",\"items\":[";
    out+=item("kernel","Running kernel",text(root,root/"proc/sys/kernel/osrelease"));
    out+=','+item("cpu-online","Online CPUs",text(root,root/"sys/devices/system/cpu/online"));
    auto memory=text(root,root/"proc/meminfo");
    if(memory.state=="available") {
        std::istringstream lines(memory.text);std::string line;long long total=-1,available=-1;
        while(std::getline(lines,line)){std::istringstream row(line);std::string key,unit;long long n;if(row>>key>>n>>unit && unit=="kB" && n>=0){if(key=="MemTotal:")total=n;else if(key=="MemAvailable:")available=n;}}
        if(total>0 && available>=0 && available<=total)memory.text=std::to_string(available/1024)+" / "+std::to_string(total/1024);
        else memory={"error","memory counters missing or invalid"};
    }
    out+=','+item("memory","Available / total memory",memory,"MiB");
    bool battery=false;Value battery_missing{"unsupported","no battery interface exported by the running kernel"};
    for(const auto& supply:entries(root,root/"sys/class/power_supply")) {
        const auto type=text(root,supply/"type");
        if(type.state!="available"){battery_missing=type;continue;}
        if(type.text!="Battery")continue;
        battery=true;
        const auto name=supply.filename().string();
        out+=','+item("battery-"+name,name+" charge",integer(root,supply/"capacity",0,100),"%");
        out+=','+item("battery-status-"+name,name+" status",text(root,supply/"status"));
    }
    if(!battery)out+=','+item("battery","Battery",battery_missing);
    bool thermal=false;
    for(const auto& zone:entries(root,root/"sys/class/thermal")) {
        if(!zone.filename().string().starts_with("thermal_zone"))continue;
        thermal=true;
        const auto type=text(root,zone/"type");out+=','+item("thermal-"+zone.filename().string(),type.state=="available"?type.text:zone.filename().string(),integer(root,zone/"temp",-100000,250000),"m°C");
    }
    if(!thermal)out+=','+item("thermal","Temperature",{"unsupported","no thermal zone interface exported by the running kernel"});
    return out+"]}";
}
}
