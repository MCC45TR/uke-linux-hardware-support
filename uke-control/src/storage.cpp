// SPDX-License-Identifier: MIT
#include "storage.hpp"
#include "sysfs.hpp"
#include <charconv>
#include <map>
#include <sstream>
#include <sys/stat.h>
#include <sys/sysmacros.h>
namespace uke::storage {
namespace {
using namespace uke::sysfs;
std::optional<std::string> read(const fs::path& path,size_t limit=4096) {
    std::ifstream in(path);if(!in)return {};
    std::string data;char c;
    while(data.size()<limit && in.get(c))data+=c;
    if(in.get(c) || in.bad())return {};
    while(!data.empty() && data.back()=='\n')data.pop_back();
    return data;
}
bool number(const std::string& value) {
    uint64_t n{};auto [p,e]=std::from_chars(value.data(),value.data()+value.size(),n);
    return !value.empty() && e==std::errc{} && p==value.data()+value.size();
}
bool identity(const std::string& value) {
    auto colon=value.find(':');return colon!=std::string::npos && number(value.substr(0,colon)) && number(value.substr(colon+1));
}
struct Mount {std::string target,type;};
std::optional<std::map<std::string,std::vector<Mount>>> mounts(const fs::path& root) {
    auto text=read(root/"proc/self/mountinfo",1024*1024);if(!text)return {};
    std::map<std::string,std::vector<Mount>> result;
    std::istringstream lines(*text);std::string line;
    while(std::getline(lines,line)) {
        std::istringstream row(line);std::string id,parent,dev,source,target,opts,field,type;
        if(!(row>>id>>parent>>dev>>source>>target>>opts) || !identity(dev))return {};
        while(row>>field && field!="-"){}
        if(field!="-" || !(row>>type))return {};
        result[dev].push_back({target,type});
    }
    if(result.empty())return {};
    return result;
}
bool safe_name(const std::string& value) {
    return !value.empty() && value.size()<128 && std::all_of(value.begin(),value.end(),[](unsigned char c){
        return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='-'||c=='_'||c=='.';
    }) && value!="." && value!="..";
}
}
std::string snapshot(const std::filesystem::path& given,const std::string& selection) {
    const auto root=fs::canonical(given);
    std::string selected;
    if(!selection.empty()) {
        if(!selection.starts_with("/dev/") || !safe_name(selection.substr(5)))
            throw std::runtime_error("Select an exact kernel device path such as /dev/sda; aliases and path traversal are not accepted");
        selected=selection.substr(5);
    }
    if(!exists(root,root/"sys/class/block"))throw std::runtime_error("Kernel block inventory is unavailable");
    auto all=entries(root,root/"sys/class/block");
    std::optional<fs::path> selected_path;
    for(const auto& item:all)if(item.filename()==selected)selected_path=resolved(root,item);
    if(!selected.empty() && !selected_path)throw std::runtime_error("Selected block device is absent; refresh the inventory");
    const auto live=mounts(root);
    const auto swaps=read(root/"proc/swaps",1024*1024);
    // Any active swap blocks planning globally until dependency tracing exists.
    const bool swap_clear=swaps && swaps->starts_with("Filename") && swaps->find('\n')==std::string::npos;
    std::string out="{\"schema\":\"org.uke.control.storage.v1\",\"mode\":\"inventory_only\",\"physical_acceptance\":null,\"mutation_supported\":false,\"chroot_execution_supported\":false,\"selection\":"+(selection.empty()?std::string("null"):quote(selection))+",\"devices\":[";
    bool comma=false;
    for(const auto& item:all) {
        const auto name=item.filename().string();if(!safe_name(name))continue;
        const auto path=resolved(root,item);if(!path)throw std::runtime_error("Block device disappeared during discovery");
        if(selected_path && *path!=*selected_path && !beneath(*selected_path,*path))continue;
        auto dev=read(*path/"dev");if(!dev || !identity(*dev))throw std::runtime_error("Block device identity is unavailable");
        const bool partition=exists(root,*path/"partition");
        auto ro=read(*path/"ro");auto sectors=read(*path/"size");
        auto holders=entries(root,*path/"holders");
        const bool holders_known=exists(root,*path/"holders");
        struct stat st{};
        const bool node_ok=::lstat((root/"dev"/name).c_str(),&st)==0 && S_ISBLK(st.st_mode) &&
            *dev==std::to_string(major(st.st_rdev))+":"+std::to_string(minor(st.st_rdev));
        std::vector<Mount> mounted;
        if(live) {auto found=live->find(*dev);if(found!=live->end())mounted=found->second;}
        std::string reason=!node_ok?"device_identity_unverified":!partition?"whole_disk_is_not_a_root_filesystem":
            !live?"mount_inventory_unknown":!mounted.empty()?"mounted_device":!swap_clear?"swap_dependencies_unknown":
            !holders_known?"holder_inventory_unknown":!holders.empty()?"device_has_holders":
            ro!=std::optional<std::string>("0")?"read_only_or_unknown":"filesystem_inspection_required";
        // Re-read identity before publishing even this non-authoritative snapshot.
        if(read(*path/"dev")!=dev || resolved(root,item)!=path)throw std::runtime_error("Block selection changed; refresh required");
        if(comma)out+=',';
        comma=true;
        out+="{\"path\":"+quote("/dev/"+name)+",\"major_minor\":"+quote(*dev)+",\"identity_verified\":"+(node_ok?"true":"false")+
            ",\"kind\":"+quote(partition?"partition":"disk_or_mapping")+",\"sectors_512\":"+(sectors&&number(*sectors)?quote(*sectors):"null")+
            ",\"filesystem\":null,\"system\":null,\"chroot_ready\":false,\"reason\":"+quote(reason)+",\"mounts\":[";
        bool mc=false;for(const auto& mount:mounted){if(mc)out+=',';mc=true;out+="{\"target_mountinfo_encoded\":"+quote(mount.target)+",\"filesystem\":"+quote(mount.type)+"}";}
        out+="]}";
    }
    return out+"]}";
}
}
