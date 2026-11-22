// SPDX-License-Identifier: MIT
#pragma once
#include <sstream>
#include <stdexcept>
#include <string>

namespace uke::packages {
struct FedoraRelease { std::string target, version; };
// Parse data only; os-release must never be executed as a shell script.
inline FedoraRelease fedora_release(const std::string& text) {
    std::istringstream input(text);std::string line,id,version,name;
    while(std::getline(input,line)) {
        auto separator=line.find('=');if(separator==std::string::npos)continue;
        const auto key=line.substr(0,separator);auto value=line.substr(separator+1);
        if(value.size()>=2&&((value.front()=='"'&&value.back()=='"')||(value.front()=='\''&&value.back()=='\'')))value=value.substr(1,value.size()-2);
        if(key=="ID")id=value;else if(key=="VERSION_ID")version=value;else if(key=="VERSION")name=value;
    }
    if(id!="fedora")throw std::runtime_error("Channel changes require Fedora");
    const bool numbered=!version.empty()&&version.find_first_not_of("0123456789")==std::string::npos;
    if(version=="rawhide"||(numbered&&(name=="Rawhide"||name==version+" (Rawhide)"||name==version+" (Rawhide Prerelease)")))return {"rawhide",version};
    if(version=="44"||version=="45")return {version,version};
    throw std::runtime_error("Unsupported Fedora release; channel switching is not a distribution upgrade");
}
inline std::string fedora_target(const std::string& text) { return fedora_release(text).target; }
inline std::string fedora_dist(const FedoraRelease& release,const std::string& releasever) {
    const auto numbered=[](const std::string& value){return !value.empty()&&value.find_first_not_of("0123456789")==std::string::npos;};
    if(releasever=="rawhide"&&release.target=="rawhide"&&numbered(release.version))return ".fc"+release.version;
    if(!numbered(releasever))throw std::runtime_error("Cannot resolve the Fedora RPM dist tag");
    if((numbered(release.version)&&releasever!=release.version)||(release.target!="rawhide"&&releasever!=release.target))
        throw std::runtime_error("Installed Fedora release disagrees with DNF releasever");
    return ".fc"+releasever;
}
inline bool fedora_package_release(const std::string& value,const std::string& dist) {
    const auto start=value.rfind(".fc");
    if(start==std::string::npos)return false;
    const auto tag=value.substr(start);
    return tag==dist||tag.starts_with(dist+".");
}
}
