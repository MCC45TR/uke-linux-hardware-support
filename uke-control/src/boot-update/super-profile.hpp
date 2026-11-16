// SPDX-License-Identifier: MIT
// Pure boot-header policy. No block device, key, firmware or resume operation.
#pragma once
#include <algorithm>
#include <cstdint>
#include <optional>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace uke_boot_profile {
inline void require(bool value,const char* message) {
    if(!value)throw std::runtime_error(message);
}
struct Command {
    bool userdata=false;
    bool super=false;
    bool vrr30=false;
    std::optional<unsigned> slot;
};
inline std::string format(const Command& profile) {
    if(!profile.userdata) {
        require(!profile.super&&!profile.vrr30&&!profile.slot,"Linux header has userdata options");
        return {};
    }
    std::string result="senemos.root=userdata";
    if(profile.super)result+=" senemos.super=seed4-v1";
    if(profile.vrr30)result+=" msm.o82_vrr_test_min_hz=30";
    if(profile.slot) {
        require(*profile.slot<2,"Invalid boot slot annotation");
        result+=*profile.slot?" androidboot.slot_suffix=_b":" androidboot.slot_suffix=_a";
    }
    return result;
}
inline Command parse(const std::string& text) {
    require(text.size()<1536&&text.find('\0')==text.npos,"Invalid boot command length");
    if(text.empty())return {};
    Command profile;
    std::istringstream input(text);
    std::string argument;
    require(bool(input>>argument)&&argument=="senemos.root=userdata","Unexpected boot root selector");
    profile.userdata=true;
    while(input>>argument) {
        if(argument=="senemos.super=seed4-v1") {
            require(!profile.super,"Duplicate super profile");profile.super=true;
        }else if(argument=="msm.o82_vrr_test_min_hz=30") {
            require(!profile.vrr30,"Duplicate VRR selector");profile.vrr30=true;
        }else if(argument=="androidboot.slot_suffix=_a"||argument=="androidboot.slot_suffix=_b") {
            require(!profile.slot,"Duplicate boot slot annotation");
            profile.slot=argument.back()=='b'?1u:0u;
        }else throw std::runtime_error("Unexpected boot command line");
    }
    require(format(profile)==text,"Noncanonical boot command line");
    return profile;
}
inline bool capable(std::span<const uint8_t> image) {
    const std::string descriptor="super-root-v1;role=rescue;root=dm-uke-rescue;readonly;noresume;seed=4GiB";
    std::vector<uint8_t> note{4,0,0,0,static_cast<uint8_t>(descriptor.size()+1),0,0,0,3,0x45,0x4b,0x55,'U','K','E',0};
    note.insert(note.end(),descriptor.begin(),descriptor.end());
    note.push_back(0);note.resize((note.size()+3)/4*4,0);
    const auto found=std::search(image.begin(),image.end(),note.begin(),note.end());
    if(found==image.end()||(found-image.begin())%4||
       std::search(found+1,image.end(),note.begin(),note.end())!=image.end())return false;
    const auto text=std::search(image.begin(),image.end(),descriptor.begin(),descriptor.end());
    return text==found+16&&std::search(text+1,image.end(),descriptor.begin(),descriptor.end())==image.end();
}
inline void admit(const Command& profile,std::span<const uint8_t> image) {
    if(profile.super||profile.slot)
        require(capable(image),"Kernel lacks the exact unique aligned seed4 boot capability");
}
inline void set(std::span<uint8_t> bytes,const Command& profile,std::span<const uint8_t> image) {
    require(bytes.size()>=4096,"Truncated boot header");
    admit(profile,image);
    const auto text=format(profile);
    std::fill(bytes.begin()+44,bytes.begin()+1580,0);
    std::copy(text.begin(),text.end(),bytes.begin()+44);
}
inline bool requested(const std::string& text) {
    require(text.size()<=8192&&text.find('\0')==text.npos,"Invalid running command line");
    std::istringstream input(text);
    std::string argument,root;
    unsigned supers=0,roots=0,roles=0,rw=0,ro=0;
    while(input>>argument) {
        if(argument.starts_with("senemos.super=")) {
            require(argument=="senemos.super=seed4-v1"&&++supers==1,"Unknown or duplicate running super profile");
        }else if(argument.starts_with("senemos.role="))++roles;
        else if(argument.starts_with("root=")){++roots;root=argument;}
        else if(argument=="rw")++rw;
        else if(argument=="ro")++ro;
    }
    if(!supers)return false;
    require(roots==1&&root=="root=PARTLABEL=userdata"&&rw==1&&ro==0&&roles==0,
            "Super profile is not a writable normal userdata boot");
    return true;
}
inline void slot_matches(const Command& profile,unsigned physical_slot) {
    require(physical_slot<2&&(!profile.slot||*profile.slot==physical_slot),
            "Boot slot annotation differs from its physical destination");
}
} // namespace uke_boot_profile
