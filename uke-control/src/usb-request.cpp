// SPDX-License-Identifier: MIT
#include "backend.hpp"
#include "usb-request.hpp"
#include <algorithm>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <thread>
#include <fcntl.h>
#include <linux/magic.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <unistd.h>

namespace fs=std::filesystem;
namespace uke::usb {
std::string request_refusal(const RequestState& s,const std::string& role) {
    if(role!="host"&&role!="device")return "INVALID_DATA_ROLE";
    if(!s.profile)return "FULL_TYPEC_PROFILE_REQUIRED";
    if(!s.port||!s.role_switch)return "PORT_ASSOCIATION_AMBIGUOUS";
    if(!s.partner)return "NO_ATTACHED_PARTNER";
    if(s.typec_role==role&&s.controller_role==role)return {};
    if(s.storage)return "USB_STORAGE_ATTACHED";
    if(s.network)return "USB_NETWORK_IN_USE";
    if(s.display)return "EXTERNAL_DISPLAY_IN_USE";
    if(s.gadget)return "GADGET_REQUIRES_OWNED_TEARDOWN";
    if((s.typec_role!="host"&&s.typec_role!="device")||
       (s.controller_role!="host"&&s.controller_role!="device"&&s.controller_role!="none"))return "ROLE_STATE_UNAVAILABLE";
    return {};
}
namespace {
struct Fd {int n=-1;~Fd(){if(n>=0)close(n);} Fd(const Fd&)=delete;Fd& operator=(const Fd&)=delete;explicit Fd(int number):n(number){}};
void need(bool value,const std::string& reason){if(!value)throw std::runtime_error(reason);}
std::string value(const fs::path& path) {
    auto out=uke::read(path.string(),4096);
    while(!out.empty()&&(out.back()=='\n'||out.back()=='\0'))out.pop_back();
    return out;
}
std::string selected(const std::string& v) {
    const auto begin=v.find('['),end=v.find(']');
    return begin!=std::string::npos&&end>begin?v.substr(begin+1,end-begin-1):v;
}
std::vector<fs::path> children(const fs::path& path) {
    std::vector<fs::path> out;std::error_code error;
    for(fs::directory_iterator i(path,error),end;!error&&i!=end;i.increment(error)) {
        need(out.size()<256,"CLASS_INVENTORY_TOO_LARGE");out.push_back(i->path());
    }
    if(error&&error!=std::errc::no_such_file_or_directory)throw std::runtime_error("CLASS_INVENTORY_FAILED");
    std::sort(out.begin(),out.end());return out;
}
bool port_name(const std::string& name) {
    return name.starts_with("port")&&name.size()>4&&name.find_first_not_of("0123456789",4)==std::string::npos;
}
bool usb_ancestor(fs::path path) {
    std::error_code error;path=fs::canonical(path,error);
    if(error)return false;
    while(path!="/"&&!path.empty()) {
        if(fs::exists(path/"idVendor"))return true;
        path=path.parent_path();
    }
    return false;
}
bool full_controller(fs::path path) {
    std::error_code error;path=fs::canonical(path,error);
    if(error)return false;
    while(path!="/"&&!path.empty()) {
        const auto compatible=uke::read((path/"of_node/compatible").string(),4096);
        if(compatible.find(std::string("qcom,sm7675-dwc3-typec\0",23))!=std::string::npos)return true;
        path=path.parent_path();
    }
    return false;
}
struct Graph {
    RequestState state;
    fs::path port,role,partner;
    dev_t partner_device{};ino_t partner_inode{};
};
Graph graph() {
    Graph g;
    const auto compatible=uke::read("/sys/firmware/devicetree/base/compatible",4096);
    g.state.profile=compatible.find(std::string("xiaomi,uke\0",11))!=std::string::npos&&
        compatible.find(std::string("qcom,sm7675\0",12))!=std::string::npos&&
        value("/sys/firmware/devicetree/base/chosen/senemos,usb-profile")=="typec-v1";
    auto ports=children("/sys/class/typec");
    std::erase_if(ports,[](const fs::path& p){return !port_name(p.filename().string());});
    const auto roles=children("/sys/class/usb_role");
    g.state.port=ports.size()==1;g.state.role_switch=roles.size()==1;
    if(g.state.port) {
        g.port=fs::canonical(ports.front());g.partner=g.port/(ports.front().filename().string()+"-partner");
        struct stat st{};g.state.partner=stat(g.partner.c_str(),&st)==0;
        g.partner_device=st.st_dev;g.partner_inode=st.st_ino;
        g.state.typec_role=selected(value(g.port/"data_role"));
    }
    if(g.state.role_switch) {
        g.role=fs::canonical(roles.front());
        // The full Uke profile admits only this controller, independently of class numbering.
        g.state.role_switch=full_controller(g.role);
        g.state.controller_role=value(g.role/"role");
    }
    for(const auto& p:children("/sys/class/block"))if(usb_ancestor(p))g.state.storage=true;
    for(const auto& p:children("/sys/class/net"))if(usb_ancestor(p/"device")) {
        const auto raw=value(p/"flags");unsigned flags{};
        std::string_view v=raw;if(v.starts_with("0x"))v.remove_prefix(2);
        const auto parsed=std::from_chars(v.data(),v.data()+v.size(),flags,16);
        if(parsed.ec!=std::errc{}||parsed.ptr!=v.data()+v.size()||(flags&1))g.state.network=true;
    }
    for(const auto& p:children("/sys/class/drm")) {
        const auto name=p.filename().string();
        if((name.find("-DP-")!=std::string::npos||name.find("-HDMI-A-")!=std::string::npos)&&value(p/"enabled")=="enabled")g.state.display=true;
    }
    for(const auto& p:children("/sys/class/udc")) {
        const auto function=value(p/"function");
        if(!function.empty()&&function!="unavailable")g.state.gadget=true;
    }
    return g;
}
bool same_partner(const Graph& a,const Graph& b) {
    return a.state.partner&&b.state.partner&&a.port==b.port&&a.role==b.role&&
        a.partner_device==b.partner_device&&a.partner_inode==b.partner_inode;
}
struct GadgetLock {
    Fd fd{-1};
    GadgetLock() {
        constexpr auto directory="/run/senemos-uke";
        if(mkdir(directory,0700)<0)need(errno==EEXIST,"GADGET_STATE_UNAVAILABLE");
        struct stat st{};need(lstat(directory,&st)==0&&S_ISDIR(st.st_mode)&&st.st_uid==0&&!(st.st_mode&0022),"UNTRUSTED_GADGET_STATE");
        fd.n=open("/run/senemos-uke/usb-gadget.lock",O_CREAT|O_RDWR|O_NOFOLLOW|O_CLOEXEC,0600);
        need(fd.n>=0&&fstat(fd.n,&st)==0&&S_ISREG(st.st_mode)&&st.st_uid==0&&!(st.st_mode&0022),"UNTRUSTED_GADGET_LOCK");
        acquire();
    }
    void acquire(){need(flock(fd.n,LOCK_EX|LOCK_NB)==0,"USB_OWNER_BUSY");}
    void release(){need(flock(fd.n,LOCK_UN)==0,"USB_LOCK_FAILED");}
};
struct Inhibit {
    static constexpr auto path="/run/senemos-uke/usb-gadget-inhibit";
    Fd fd{open(path,O_CREAT|O_EXCL|O_WRONLY|O_NOFOLLOW|O_CLOEXEC,0600)};
    Inhibit(){need(fd.n>=0,"USB_TRANSITION_ALREADY_INHIBITED");}
    ~Inhibit() {
        if(fd.n<0)return;
        struct stat own{},live{};
        if(fstat(fd.n,&own)==0&&lstat(path,&live)==0&&own.st_dev==live.st_dev&&own.st_ino==live.st_ino)unlink(path);
    }
};
std::string escaped(const std::string& text) {
    std::string out;
    for(unsigned char c:text)if(c>=32&&c<127&&c!='"'&&c!='\\')out+=char(c);else out+='?';
    return out;
}
}
Result request_data(const std::string& desired) {
    bool accepted=false,observed=false,noop=false;
    std::string reason,before_typec,before_controller,after_typec,after_controller;
    int status=69;
    try {
        need(geteuid()==0,"ROOT_REQUIRED");
        auto first=graph();
        before_typec=first.state.typec_role;before_controller=first.state.controller_role;
        auto plan=first.state;plan.gadget=false;
        need(request_refusal(plan,desired).empty(),request_refusal(plan,desired));
        noop=first.state.typec_role==desired&&first.state.controller_role==desired;
        if(noop) {observed=true;status=0;after_typec=before_typec;after_controller=before_controller;}
        else {
            const auto api=execute({"/usr/bin/rpm","-q","--whatprovides","uke-usb-gadget-ownership-api = 2"},5,4096);
            need(api.status==0,"GADGET_OWNERSHIP_API2_REQUIRED");
            GadgetLock lock;Inhibit inhibit;
            auto current=graph();need(same_partner(first,current),"ATTACHMENT_CHANGED");
            if(current.state.gadget) {
                need(value("/run/senemos-uke/usb-gadget-profile")=="auto-acm","FOREIGN_OR_LEGACY_GADGET_BOUND");
                lock.release();
                const auto cleanup=execute({"/usr/libexec/senemos-uke/uke-usb-network","stop-auto"},10,4096);
                lock.acquire();need(cleanup.status==0,"OWNED_GADGET_TEARDOWN_FAILED");
            }
            current=graph();need(same_partner(first,current),"ATTACHMENT_CHANGED");
            need(request_refusal(current.state,desired).empty(),request_refusal(current.state,desired));
            Fd fd{open((current.port/"data_role").c_str(),O_WRONLY|O_CLOEXEC|O_NOFOLLOW)};
            struct statfs filesystem{};need(fd.n>=0&&fstatfs(fd.n,&filesystem)==0&&filesystem.f_type==SYSFS_MAGIC,"TYPEC_SYSFS_NOT_WRITABLE");
            const auto data=desired+'\n';
            const auto written=write(fd.n,data.data(),data.size());
            if(written!=ssize_t(data.size()))throw std::runtime_error("TYPEC_REQUEST_FAILED: "+(written<0?std::string(strerror(errno)):"short write"));
            accepted=true;
            const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(10);
            unsigned matches=0;
            while(std::chrono::steady_clock::now()<deadline) {
                current=graph();after_typec=current.state.typec_role;after_controller=current.state.controller_role;
                need(same_partner(first,current),"ATTACHMENT_CHANGED");
                if(after_typec==desired&&after_controller==desired)++matches;else matches=0;
                if(matches==3){observed=true;status=0;break;}
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            if(!observed){reason="ROLE_CONVERGENCE_TIMEOUT";status=124;}
        }
    }catch(const std::exception& e){reason=e.what();}
    const auto boolean=[](bool b){return b?"true":"false";};
    std::string out="{\"schema\":\"org.uke.control.usb.request.v1\",\"requested_data_role\":\""+escaped(desired)+'"';
    out+=",\"write_accepted\":"+std::string(boolean(accepted))+",\"observed_match\":"+boolean(observed)+",\"already_matching\":"+boolean(noop);
    out+=",\"before_typec\":\""+escaped(before_typec)+"\",\"before_controller\":\""+escaped(before_controller)+'"';
    out+=",\"after_typec\":\""+escaped(after_typec)+"\",\"after_controller\":\""+escaped(after_controller)+'"';
    out+=",\"error\":"+(reason.empty()?std::string("null"):"\""+escaped(reason)+"\"");
    out+=",\"physical_validation\":\"NOT_ESTABLISHED\",\"persisted\":false}\n";
    return {status,out};
}
}
