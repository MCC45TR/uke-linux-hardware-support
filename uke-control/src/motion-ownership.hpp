// SPDX-License-Identifier: MIT
#pragma once
#include <charconv>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace uke::motion {
struct Setting {std::string path,before,after;};

// The backing store must durably replace the entire journal before the first
// write, retain it on restoration failure and serialize owners with the IIO fd.
// Paths are relative, fixed ABI attributes. No caller supplies filesystem paths.
inline bool setting_path(std::string_view path) {
    if(path=="buffer/enable"||path=="buffer/length"||path=="buffer/watermark"||path=="current_timestamp_clock")return true;
    if(!path.starts_with("scan_elements/")||!path.ends_with("_en"))return false;
    path.remove_prefix(14);path.remove_suffix(3);
    for(auto name:{"in_gravity_x","in_gravity_y","in_gravity_z","in_accel_linear_x","in_accel_linear_y","in_accel_linear_z","in_timestamp"})if(path==name)return true;
    return false;
}
inline bool settings_valid(const std::vector<Setting>& settings) {
    if(settings.empty()||settings.size()>11)return false;
    for(std::size_t i=0;i<settings.size();++i){
        const auto& s=settings[i];
        if(!setting_path(s.path)||s.before.empty()||s.after.empty()||s.before.size()>64||s.after.size()>64||
           s.before.find_first_of("\r\n\0",0,3)!=s.before.npos||s.after.find_first_of("\r\n\0",0,3)!=s.after.npos)return false;
        if(s.path=="current_timestamp_clock"){
            bool known=false;
            for(auto clock:{"monotonic","monotonic_raw","realtime","boottime","realtime_coarse","monotonic_coarse","tai"})known|=s.before==clock;
            if(!known||s.after!="boottime")return false;
        }else if(s.path=="buffer/length"||s.path=="buffer/watermark"){
            unsigned long long value=0;const auto result=std::from_chars(s.before.data(),s.before.data()+s.before.size(),value);
            if(result.ec!=std::errc()||result.ptr!=s.before.data()+s.before.size()||!value||s.after!=(s.path=="buffer/length"?"4":"1"))return false;
        }else if((s.before!="0"&&s.before!="1")||s.after!="1")return false;
        for(std::size_t j=0;j<i;++j)if(settings[j].path==s.path)return false;
    }
    return settings.back().path=="buffer/enable"&&settings.back().before=="0"&&settings.back().after=="1";
}
struct Store {
    virtual ~Store()=default;
    virtual std::optional<std::string> read(std::string_view)=0;
    virtual bool write(std::string_view,std::string_view)=0;
    virtual bool save(const std::vector<Setting>&)=0;
    virtual bool clear()=0;
};
// Recovery is conservative: a foreign value prevents every write. The caller
// must hold the matching device's exclusive fd, including during crash recovery.
inline bool restore(Store& store,const std::vector<Setting>& settings) {
    if(!settings_valid(settings))return false;
    for(const auto& s:settings){
        const auto value=store.read(s.path);
        if(!value||(*value!=s.before&&*value!=s.after))return false;
    }
    for(auto it=settings.rbegin();it!=settings.rend();++it){
        const auto value=store.read(it->path);
        if(!value||(*value!=it->before&&*value!=it->after))return false;
        if(*value!=it->before&&(!store.write(it->path,it->before)||store.read(it->path)!=it->before))return false;
    }
    return store.clear();
}
inline bool configure(Store& store,const std::vector<Setting>& settings) {
    if(!settings_valid(settings))return false;
    for(const auto& s:settings)if(store.read(s.path)!=s.before)return false;
    if(!store.save(settings))return false;
    for(const auto& s:settings){
        if(store.read(s.path)!=s.before||!store.write(s.path,s.after)||store.read(s.path)!=s.after){
            restore(store,settings);return false;
        }
    }
    return true;
}
}
