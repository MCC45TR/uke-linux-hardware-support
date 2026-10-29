// SPDX-License-Identifier: MIT
#include "nfc.hpp"
#include "sysfs.hpp"
#include <charconv>
namespace uke::nfc {
namespace fs=std::filesystem;
namespace {
std::optional<unsigned> index(const std::string& name,const std::string& prefix) {
    if(!name.starts_with(prefix))return {};
    unsigned n=0;const auto begin=name.data()+prefix.size(),end=name.data()+name.size();auto r=std::from_chars(begin,end,n);
    return r.ec==std::errc{}&&r.ptr==end?std::optional<unsigned>(n):std::nullopt;
}
std::optional<bool> bit(const fs::path& sys,const fs::path& file) {
    const auto value=sysfs::attribute(sys,file);if(value=="\"0\"")return false;if(value=="\"1\"")return true;return {};
}
std::string boolean(std::optional<bool> value){return value?(*value?"true":"false"):"null";}
}
std::vector<Radio> radios(const fs::path& sys) {
    std::vector<Radio> result;const auto devices=sysfs::resolved(sys,sys/"devices");if(!devices)return result;
    const auto switches=sysfs::entries(sys,sys/"class/rfkill");
    for(const auto& entry:sysfs::entries(sys,sys/"class/nfc")) {
        if(!index(entry.filename().string(),"nfc"))continue;
        auto device=sysfs::resolved(*devices,entry);if(!device)continue;
        Radio radio{entry.filename().string(),*device,{},{},{}};unsigned matches=0;
        for(const auto& sw:switches) {
            auto n=index(sw.filename().string(),"rfkill");auto path=sysfs::resolved(*device,sw);
            if(!n||!path||sysfs::attribute(sys,*path/"type")!="\"nfc\"")continue;
            ++matches;radio.index=n;radio.soft=bit(sys,*path/"soft");radio.hard=bit(sys,*path/"hard");
        }
        if(matches!=1){radio.index.reset();radio.soft.reset();radio.hard.reset();}result.push_back(radio);
    }return result;
}
rfkill_event request(const std::vector<Radio>& radios,bool enabled) {
    if(radios.empty())throw std::runtime_error("No active Linux NFC controller is available; a tap-to-share feature does not establish a controllable reader");
    if(radios.size()!=1)throw std::runtime_error("Multiple NFC controllers are present; refusing an ambiguous radio change");
    const auto& radio=radios[0];
    if(!radio.index||!radio.soft||!radio.hard)throw std::runtime_error("The NFC controller has no unique readable rfkill control");
    if(enabled&&*radio.hard)throw std::runtime_error("NFC is hardware-blocked");
    rfkill_event event{};event.idx=*radio.index;event.type=RFKILL_TYPE_NFC;event.op=RFKILL_OP_CHANGE;event.soft=!enabled;return event;
}
std::string snapshot(const fs::path& sys) {
    auto found=radios(sys);std::string out="{\"schema\":\"org.uke.control.nfc.v1\",\"scope\":\"rfkill_radio_permission\",\"available\":"+std::string(found.empty()?"false":"true")+",\"powered\":null,\"polling_started\":false,\"reason\":"+(found.empty()?sysfs::quote("no_active_linux_nfc_controller"):"null")+",\"adapters\":[";
    bool first=true;for(const auto& radio:found){if(!first)out+=',';first=false;
        out+="{\"name\":"+sysfs::quote(radio.name)+",\"control_available\":"+((found.size()==1&&radio.index&&radio.soft&&radio.hard)?"true":"false")+",\"soft_blocked\":"+boolean(radio.soft)+",\"hard_blocked\":"+boolean(radio.hard)+",\"radio_permitted\":"+boolean(radio.soft&&radio.hard?std::optional<bool>(!*radio.soft&&!*radio.hard):std::nullopt)+'}';
    }out+="],\"configured_tag_interfaces\":[";first=true;
    const auto devices=sysfs::resolved(sys,sys/"devices");
    if(devices)for(const auto& entry:sysfs::entries(sys,sys/"bus/i2c/devices")){
        const auto target=sysfs::resolved(*devices,entry);
        if(!target||sysfs::attribute(sys,*target/"of_node/compatible")!="\"fm,fm19511\"")continue;
        if(!first)out+=',';
        first=false;
        out+="{\"name\":"+sysfs::quote(entry.filename().string())+",\"compatible\":\"fm,fm19511\",\"driver\":"+sysfs::driver(sys,*target)+",\"tag_memory_writer_available\":false}";
    }
    return out+"]}\n";
}
}
