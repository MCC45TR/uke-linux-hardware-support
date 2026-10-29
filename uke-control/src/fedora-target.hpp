// SPDX-License-Identifier: MIT
#pragma once
#include <sstream>
#include <stdexcept>
#include <string>

namespace uke::packages {
// Parse data only; os-release must never be executed as a shell script.
inline std::string fedora_target(const std::string& text) {
    std::istringstream input(text);std::string line,id,version,name;
    while(std::getline(input,line)) {
        auto separator=line.find('=');if(separator==std::string::npos)continue;
        const auto key=line.substr(0,separator);auto value=line.substr(separator+1);
        if(value.size()>=2&&((value.front()=='"'&&value.back()=='"')||(value.front()=='\''&&value.back()=='\'')))value=value.substr(1,value.size()-2);
        if(key=="ID")id=value;else if(key=="VERSION_ID")version=value;else if(key=="VERSION")name=value;
    }
    if(id!="fedora")throw std::runtime_error("Channel changes require Fedora");
    if(version=="rawhide"||name=="Rawhide")return "rawhide";
    if(version=="44"||version=="45")return version;
    throw std::runtime_error("Unsupported Fedora release; channel switching is not a distribution upgrade");
}
inline bool fedora_package_release(const std::string& value,const std::string& dist) {
    const auto start=value.rfind(".fc");
    if(start==std::string::npos)return false;
    const auto tag=value.substr(start);
    return tag==dist||tag.starts_with(dist+".");
}
}
