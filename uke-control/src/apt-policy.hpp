// SPDX-License-Identifier: MIT
#pragma once
#include <algorithm>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

// Read-only policy primitives. These do not authenticate an archive, solve
// dependencies, authorize a transaction, or access the running system.
namespace uke::apt {
inline void require(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}
inline std::string trim(std::string value) {
    const auto first=value.find_first_not_of(" \t\r");
    if(first==std::string::npos) return {};
    return value.substr(first,value.find_last_not_of(" \t\r")-first+1);
}
using Stanza=std::map<std::string,std::string>;
inline std::vector<Stanza> sources(const std::string& text) {
    require(text.size()<=1024*1024,"Source configuration exceeds limit");
    require(text.find('\0')==std::string::npos,"NUL in source configuration");
    std::istringstream input(text);
    std::vector<Stanza> result;
    Stanza current;
    std::string line, previous;
    const auto finish=[&] {
        if(!current.empty()) result.push_back(current);
        current.clear(); previous.clear();
        require(result.size()<=256,"Too many source stanzas");
    };
    while(std::getline(input,line)) {
        require(line.size()<=8192,"Source line exceeds limit");
        if(trim(line).empty()) { finish(); continue; }
        if(line.front()=='#') continue;
        if(line.front()==' ' || line.front()=='\t') {
            require(!previous.empty(),"Orphan continuation");
            current[previous]+=" "+trim(line); continue;
        }
        const auto colon=line.find(':');
        require(colon!=std::string::npos && colon>0,"Invalid source field");
        auto key=line.substr(0,colon);
        for(char& c:key) {
            require((c>='A'&&c<='Z')||(c>='a'&&c<='z')||c=='-',"Invalid field name");
            if(c>='A'&&c<='Z') c=char(c-'A'+'a');
        }
        require(!current.contains(key),"Duplicate source field");
        current[key]=trim(line.substr(colon+1)); previous=key;
    }
    finish(); return result;
}
inline std::string field(const Stanza& stanza,const std::string& key) {
    auto found=stanza.find(key); return found==stanza.end()?std::string{}:found->second;
}
inline std::string inspect_managed_sources(const std::string& text) {
    const std::set<std::string> allowed{"types","uris","suites","components",
        "architectures","signed-by","enabled"};
    std::string selected;
    for(const auto& stanza:sources(text)) {
        for(const auto& [key,value]:stanza) {
            (void)value;
            require(allowed.contains(key),"Unsupported or unsafe managed source option");
        }
        const auto enabled=field(stanza,"enabled");
        require(enabled.empty()||enabled=="yes"||enabled=="no","Invalid Enabled field");
        require(field(stanza,"types")=="deb","Managed source must contain only deb");
        require(field(stanza,"suites")=="tenacious","Wrong or multiple suites");
        require(field(stanza,"components")=="main","Wrong components");
        require(field(stanza,"architectures")=="arm64","Wrong or implicit architecture");
        require(field(stanza,"signed-by")=="/usr/share/keyrings/uke-linux-archive.gpg",
            "Explicit managed archive keyring required");
        const auto uri=field(stanza,"uris");
        std::string channel;
        if(uri=="https://ppa.launchpadcontent.net/mcs45tr/uke-linux-unstable/ubuntu/") channel="unstable";
        else if(uri=="https://ppa.launchpadcontent.net/mcs45tr/uke-linux-stable/ubuntu/") channel="stable";
        else require(false,"Unrecognized managed archive URL");
        if(enabled=="no") continue;
        require(channel!="stable","Stable channel qualification is pending");
        require(selected.empty(),"Multiple enabled managed sources");
        selected=channel;
    }
    return selected;
}
struct Installed {
    std::string name, architecture, version, want, error, status;
};
inline bool package_name(const std::string& value) {
    if(value.size()<2 || value.size()>255 || !((value[0]>='a'&&value[0]<='z')||(value[0]>='0'&&value[0]<='9'))) return false;
    return std::all_of(value.begin(),value.end(),[](char c) {
        return (c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='+'||c=='-'||c=='.';
    });
}
// Input is the documented dpkg-query --showformat output, not human dpkg -l.
inline std::vector<Installed> installed(const std::string& text) {
    require(text.size()<=16*1024*1024,"Package inventory exceeds limit");
    require(text.find('\0')==std::string::npos,"NUL in package inventory");
    std::istringstream input(text);
    std::vector<Installed> result;
    std::set<std::string> identities;
    std::string line;
    while(std::getline(input,line)) {
        require(line.size()<=8192,"Inventory line exceeds limit");
        std::vector<std::string> fields;
        std::size_t start=0, end;
        while((end=line.find('\t',start))!=std::string::npos) {
            fields.push_back(line.substr(start,end-start)); start=end+1;
        }
        fields.push_back(line.substr(start));
        require(fields.size()==6,"Inventory requires six fields");
        Installed p{fields[0],fields[1],fields[2],fields[3],fields[4],fields[5]};
        require(package_name(p.name),"Invalid package name");
        require(p.architecture=="arm64"||p.architecture=="all","Foreign package architecture");
        require(!p.version.empty()&&p.version.find_first_of(" \r\n") == std::string::npos,"Invalid package version");
        require(identities.insert(p.name+":"+p.architecture).second,"Duplicate package identity");
        require(p.want=="install"||p.want=="hold"||p.want=="deinstall"||p.want=="purge"||p.want=="unknown","Invalid desired status");
        require(p.error=="ok","dpkg reports a package error");
        require(p.status=="installed"||p.status=="config-files"||p.status=="not-installed",
            "Interrupted dpkg configuration or pending triggers require review");
        result.push_back(p);
    }
    return result;
}
inline bool boot_or_private(const std::string& name) {
    return name.starts_with("linux-")||name.starts_with("initramfs-")||
        name.starts_with("grub-")||name.starts_with("shim-")||name.starts_with("systemd-boot")||
        name.starts_with("uke-kernel")||name.starts_with("uke-boot")||name.starts_with("uke-dtb")||
        name.starts_with("uke-firmware")||name.starts_with("uke-speaker-firmware")||
        name.starts_with("uke-local-")||name.starts_with("senemos-uke-linux-kernel")||
        name=="uke-orangefox-recovery";
}
struct Change {
    std::string name, architecture, action;
};
// Early exclusions only. An empty result is NOT transaction authorization.
inline std::vector<std::string> exclusions(const std::vector<Installed>& inventory,
                                          const std::vector<Change>& changes) {
    std::set<std::string> held, seen;
    for(const auto& package:inventory)
        if(package.want=="hold") held.insert(package.name+":"+package.architecture);
    std::vector<std::string> reasons;
    for(const auto& change:changes) {
        const auto id=change.name+":"+change.architecture;
        if(!package_name(change.name)) reasons.push_back("invalid package name");
        if(change.architecture!="arm64"&&change.architecture!="all") reasons.push_back("foreign architecture: "+id);
        if(!seen.insert(id).second) reasons.push_back("duplicate plan entry: "+id);
        if(held.contains(id)) reasons.push_back("held package: "+id);
        if(boot_or_private(change.name)) reasons.push_back("boot or private payload requires dedicated path: "+id);
        if(change.action!="install"&&change.action!="upgrade") reasons.push_back("unsupported action: "+id);
    }
    return reasons;
}
} // namespace uke::apt
