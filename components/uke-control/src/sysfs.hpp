// SPDX-License-Identifier: MIT
#pragma once
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>
namespace uke::sysfs {
namespace fs=std::filesystem;
inline std::string quote(const std::string& value) {
    constexpr char hex[]="0123456789abcdef";
    std::string out="\"";
    for(size_t i=0;i<value.size();++i) {
        const auto c=static_cast<unsigned char>(value[i]);
        if(c=='"'||c=='\\'){out+='\\';out+=char(c);}
        else if(c<32||c==127){out+="\\u00";out+=hex[c>>4];out+=hex[c&15];}
        else if(c>=128) {
            const size_t length=c>=0xc2&&c<=0xdf?2:c>=0xe0&&c<=0xef?3:c>=0xf0&&c<=0xf4?4:0;
            bool valid=length && length<=value.size()-i;
            for(size_t j=1;valid&&j<length;++j) {
                const auto next=static_cast<unsigned char>(value[i+j]);
                valid=next>=0x80&&next<=0xbf;
            }
            if(valid) {
                const auto next=static_cast<unsigned char>(value[i+1]);
                valid=!(c==0xe0&&next<0xa0) && !(c==0xed&&next>=0xa0) &&
                      !(c==0xf0&&next<0x90) && !(c==0xf4&&next>=0x90);
            }
            // Kernel attributes can contain arbitrary bytes; replace each invalid byte.
            if(valid){out.append(value,i,length);i+=length-1;}
            else out+="\\ufffd";
        }
        else out+=char(c);
    }
    return out+'"';
}
inline bool beneath(const fs::path& root,const fs::path& path) {
    auto r=root.begin(),p=path.begin();
    for(;r!=root.end();++r,++p)if(p==path.end()||*r!=*p)return false;
    return true;
}
inline std::optional<fs::path> resolved(const fs::path& root,const fs::path& path) {
    std::error_code error;
    auto target=fs::canonical(path,error);
    if(error||!beneath(root,target))return std::nullopt;
    return target;
}
inline std::string attribute(const fs::path& root,const fs::path& path) {
    const auto target=resolved(root,path);
    if(!target)return "null";
    std::error_code error;
    if(!fs::is_regular_file(*target,error)||error)return "null";
    std::ifstream in(*target,std::ios::binary);
    if(!in)return "null";
    std::string result;char c;
    while(result.size()<4096&&in.get(c))result+=c;
    if(in.get(c))throw std::runtime_error("Sysfs attribute exceeds 4096 bytes");
    if(in.bad())return "null";
    while(!result.empty()&&(result.back()=='\n'||result.back()=='\0'))result.pop_back();
    return quote(result);
}
inline std::vector<fs::path> entries(const fs::path& root,const fs::path& directory) {
    std::vector<fs::path> result;
    const auto target=resolved(root,directory);
    if(!target)return result;
    std::error_code error;
    for(fs::directory_iterator i(*target,error),end;!error&&i!=end;i.increment(error)) {
        if(result.size()>=256)throw std::runtime_error("Sysfs inventory exceeds 256 class objects");
        result.push_back(i->path());
    }
    if(error)throw std::runtime_error("Sysfs inventory directory could not be read");
    std::sort(result.begin(),result.end());return result;
}
inline bool exists(const fs::path& root,const fs::path& path){return bool(resolved(root,path));}
inline std::string driver(const fs::path& root,const fs::path& object) {
    auto target=resolved(root,object/"driver");
    if(!target)target=resolved(root,object/"device/driver");
    return target?quote(target->filename().string()):"null";
}
inline std::string object(const fs::path& root,const fs::path& path,const std::vector<std::string>& fields) {
    std::string result="{\"name\":"+quote(path.filename().string())+",\"driver\":"+driver(root,path);
    for(const auto& field:fields)result+=','+quote(field=="name"?"device_name":field)+':'+attribute(root,path/field);
    return result+'}';
}
}
