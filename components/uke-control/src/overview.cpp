// SPDX-License-Identifier: MIT
// Read-only views and explicit local files; never runs code from inspected roots.
#include "overview.hpp"
#include "sysfs.hpp"
#include <iostream>
#include "backend.hpp"
#include "cli.hpp"
#include "desktop.hpp"
#include "plasma.hpp"
#include "link-policy.hpp"
#include "sensors.hpp"
#include "usb-request.hpp"
#include <json/json.h>
#include <algorithm>
#include <array>
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <fcntl.h>
#include <linux/openat2.h>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/sysmacros.h>
#include <sys/utsname.h>
#include <time.h>
#include <unistd.h>

namespace uke::overview {
namespace {
using Value=Json::Value;
namespace fs=std::filesystem;
constexpr size_t file_limit=8*1024*1024;
const std::array<std::string,15> domains{"battery","charge","display","usb","desktop","hardware","sensors","audio","camera","nfc","sleep","storage","updates","recovery","workload"};
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
uint64_t now(){timespec ts{};require(!clock_gettime(CLOCK_BOOTTIME,&ts),"Boottime unavailable");return uint64_t(ts.tv_sec)*1000000000+ts.tv_nsec;}
std::string encode(const Value& value){Json::StreamWriterBuilder writer;writer["indentation"]="";writer["emitUTF8"]=true;return Json::writeString(writer,value);}
Value parse(const std::string& input) {
    require(input.size()<=file_limit,"Snapshot exceeds 8 MiB");
    Json::CharReaderBuilder builder;builder["rejectDupKeys"]=true;builder["allowComments"]=false;
    builder["failIfExtra"]=true;builder["stackLimit"]=64;
    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());Value value;std::string error;
    require(reader->parse(input.data(),input.data()+input.size(),&value,&error),"Invalid snapshot JSON");return value;
}
struct Descriptor {int fd;~Descriptor(){if(fd>=0)close(fd);}};
std::string bytes(int fd,size_t limit) {
    struct stat st{};require(fd>=0&&!fstat(fd,&st)&&S_ISREG(st.st_mode),"A regular readable file is required");
    std::string text;std::array<char,4096> buffer{};
    while(true) {
        const auto n=::read(fd,buffer.data(),std::min(buffer.size(),limit-text.size()+1));
        if(n<0&&errno==EINTR)continue;
        require(n>=0,"Cannot read snapshot");if(!n)break;
        require(size_t(n)<=limit-text.size(),"Snapshot file exceeds its limit");text.append(buffer.data(),size_t(n));
    }
    return text;
}
Value load(const std::string& path){Descriptor fd{open(path.c_str(),O_RDONLY|O_CLOEXEC|O_NOFOLLOW|O_NONBLOCK)};return parse(bytes(fd.fd,file_limit));}
void save(const std::string& path,const Value& value) {
    const auto data=encode(value)+'\n';require(data.size()<=file_limit,"Output exceeds 8 MiB");
    Descriptor fd{open(path.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600)};
    require(fd.fd>=0,"Cannot create private output; existing files are preserved");
    require(!fchmod(fd.fd,0600),"Cannot protect output");size_t done=0;
    while(done<data.size()) {const auto n=write(fd.fd,data.data()+done,data.size()-done);if(n<0&&errno==EINTR)continue;require(n>0,"Output write failed");done+=size_t(n);}
    require(!fsync(fd.fd),"Output synchronization failed");
}
Value receipt(const std::vector<std::string>& args) {
    const auto result=execute_bounded(args,5,262144);Value value;
    value["exit_status"]=result.status;value["timed_out"]=result.timed_out;value["truncated"]=result.truncated;
    value["output"]=parse(sysfs::quote(result.output));return value;
}
Value result(const Result& response) {
    auto value=parse(response.output);
    value["query_exit_status"]=response.status;return value;
}
Value collect(const std::string& domain) {
    Value value;
    if(domain=="battery")return result(battery_query("status"));
    if(domain=="charge")return result(charge_control_query("admission"));
    if(domain=="desktop")return parse(desktop::status());
    if(domain=="usb")return parse(usb_status());
    if(domain=="hardware"||domain=="camera")return parse(hardware_status());
    if(domain=="nfc")return parse(nfc_status());
    if(domain=="storage") {
        value=parse(inventory(false));std::error_code error;const auto space=fs::space("/",error);
        value["root_capacity_bytes"]=error?Value(Json::nullValue):Value(Json::UInt64(space.capacity));
        value["root_available_bytes"]=error?Value(Json::nullValue):Value(Json::UInt64(space.available));return value;
    }
    if(domain=="sensors")return parse(sensors::json(sensors::sysfs_snapshot()));
    if(domain=="display") {
        value["stream"]=parse(links::display_bits_status("/"));
        value["profiles"]=parse(usb::display_profiles("/"));value["session"]=parse(desktop::plasma_status());
    } else if(domain=="audio") {
        value["devices"]=parse(hardware_status());value["session"]=receipt({"/usr/bin/wpctl","status","--name"});
        value["stream_started"]=false;value["recording_started"]=false;
    } else if(domain=="sleep") {
        value["wake_permissions"]=parse(wake_status());value["selected_modes"]=read("/sys/power/mem_sleep",4096);
        value["suspend_tested"]=Json::nullValue;
    } else if(domain=="updates") {
        value["channel"]=receipt({"/usr/bin/uke-control","--json","channel","status"});
        if(value["channel"]["exit_status"]==0)value["channel_info"]=parse(value["channel"]["output"].asString());
        value["boot"]=receipt({"/usr/bin/uke-boot-status"});value["userspace_rollback_available"]=false;
    } else if(domain=="recovery") {
        value["private_state_readable"]=geteuid()==0;
        if(geteuid()==0)for(const char* name:{"channel-pending","channel-operation","startup.json","charge-intent"})value[name]=read(std::string("/var/lib/uke-control/")+name,65536);
        value["layout_prepared"]=geteuid()==0?Value(fs::exists("/run/initramfs/etc/uke-layout/request.json")):Value(Json::nullValue);
        value["resize_prepared"]=geteuid()==0?Value(fs::exists("/run/initramfs/etc/uke-resize-system.request")):Value(Json::nullValue);
        value["automatic_recovery"]=false;
    } else if(domain=="workload") {
        for(const char* file:{"meminfo","pressure/cpu","pressure/memory","pressure/io"})value[file]=read(std::string("/proc/")+file,65536);
        std::error_code error;const auto space=fs::space("/",error);
        value["root_available_bytes"]=error?Value(Json::nullValue):Value(Json::UInt64(space.available));
        value["power_policy"]=parse(desktop::status());value["policy_changed"]=false;
    } else throw cli::UsageError("Unknown overview component");
    return value;
}
Value snapshot(const std::string& domain) {
    const auto begin=now();Value value;
    value["schema"]="org.uke.control.component.v1";value["component"]=domain;
    value["read_only"]=true;value["snapshot_atomic"]=false;value["physical_acceptance"]=Json::nullValue;
    value["observed_boottime_ns"]=Json::UInt64(begin);value["source_sample_age_ms"]=Json::nullValue;
    try {
        value["data"]=collect(domain);value["availability"]="available";
        const auto& data=value["data"];
        if(data.isObject()&&data["query_exit_status"].isInt()&&data["query_exit_status"].asInt()!=0)value["availability"]="error";
        if(domain=="charge"&&data["profile_qualified"].isBool()&&!data["profile_qualified"].asBool())value["availability"]="not_qualified";
    }
    catch(const std::exception& error) {value["data"]=Json::nullValue;value["availability"]="error";value["error"]["code"]="QUERY_FAILED";value["error"]["message"]=error.what();}
    value["collection_span_ms"]=Json::UInt64((now()-begin)/1000000);return value;
}
Value overview(const std::string& domain={}) {
    require(domain.empty()||std::find(domains.begin(),domains.end(),domain)!=domains.end(),"Unknown overview component");
    Value value;value["schema"]="org.uke.control.overview.v1";value["application_version"]=cli::version;
    value["device_family"]="Uke / SM7675";value["physical_acceptance"]=Json::nullValue;value["components"]=Value(Json::arrayValue);
    if(domain.empty())for(const auto& name:domains)value["components"].append(snapshot(name));
    else value["components"].append(snapshot(domain));
    return value;
}
void validate(const Value& value) {
    require(value.isObject()&&value["schema"]=="org.uke.control.overview.v1"&&value["components"].isArray()&&value["components"].size()<=domains.size(),"Expected a versioned Uke overview snapshot");
    std::set<std::string> seen;
    for(const auto& component:value["components"]) {
        const auto domain=component["component"].asString();
        require(std::find(domains.begin(),domains.end(),domain)!=domains.end()&&seen.insert(domain).second,"Unknown or duplicate snapshot component");
        const std::set<std::string> allowed{"available","unsupported","not_qualified","unknown","denied","stale","error"};
        require(component["availability"].isString()&&allowed.contains(component["availability"].asString()),"Snapshot availability is invalid");
    }
}
Value exported(const Value& input) {
    validate(input);Value value;value["schema"]="org.uke.control.support.v1";
    value["application_version"]=cli::version;value["device_family"]="Uke / SM7675";value["physical_acceptance"]=Json::nullValue;
    value["components"]=Value(Json::arrayValue);
    for(const auto& component:input["components"]) {
        Value safe;safe["component"]=component["component"];
        const std::set<std::string> allowed{"available","unsupported","not_qualified","unknown","denied","stale","error"};
        const auto availability=component["availability"].asString();safe["availability"]=allowed.contains(availability)?availability:"unknown";
        value["components"].append(safe);
    }
    return value;
}
std::string pointer(const std::string& key) {
    std::string out;for(char c:key)out+=c=='~'?"~0":c=='/'?"~1":std::string(1,c);return out;
}
using Fields=std::map<std::string,Value>;
void flatten(const Value& value,const std::string& path,Fields& fields,unsigned depth=0) {
    require(depth<=32&&fields.size()<50000,"Comparison exceeds its structural bound");
    static const std::set<std::string> volatile_fields{"observed_boottime_ns","observed_monotonic_ns","observed_monotonic_us","collection_span_ms","collection_started_ns","collection_finished_ns","timestamp","pid"};
    if(value.isObject()&&!value.empty())for(const auto& name:value.getMemberNames())if(!volatile_fields.contains(name))flatten(value[name],path+'/'+pointer(name),fields,depth+1);
    if(value.isArray()&&!value.empty())for(Json::ArrayIndex i=0;i<value.size();++i)flatten(value[i],path+'/'+std::to_string(i),fields,depth+1);
    if((!value.isArray()&&!value.isObject())||value.empty())fields[path]=value;
}
Value compare(const Value& before,const Value& after) {
    validate(before);validate(after);Fields a,b;
    for(const auto& component:before["components"])flatten(component,'/'+component["component"].asString(),a);
    for(const auto& component:after["components"])flatten(component,'/'+component["component"].asString(),b);
    Value value;value["schema"]="org.uke.control.comparison.v1";value["causality_established"]=false;value["changes"]=Value(Json::arrayValue);
    std::set<std::string> keys;for(const auto& [key,item]:a){(void)item;keys.insert(key);}for(const auto& [key,item]:b){(void)item;keys.insert(key);}
    for(const auto& key:keys)if(!a.contains(key)||!b.contains(key)||a.at(key)!=b.at(key)) {
        Value change;change["path"]=key;change["before_present"]=a.contains(key);change["after_present"]=b.contains(key);
        change["before"]=a.contains(key)?a.at(key):Value(Json::nullValue);change["after"]=b.contains(key)?b.at(key):Value(Json::nullValue);
        value["changes"].append(change);
    }
    value["before_observations"]=Value(Json::arrayValue);value["after_observations"]=Value(Json::arrayValue);
    for(const auto& input:std::array{std::pair{&before,"before_observations"},std::pair{&after,"after_observations"}})
        for(const auto& component:(*input.first)["components"]) {Value observation;observation["component"]=component["component"];observation["observed_boottime_ns"]=component["observed_boottime_ns"];value[input.second].append(observation);}
    return value;
}
Value roots() {
    const auto response=execute_bounded({"/usr/bin/lsblk","--json","--bytes","--output","NAME,TYPE,PATH,MAJ:MIN,PARTUUID,UUID,FSTYPE,SIZE,MOUNTPOINTS"},5,1024*1024);
    require(!response.status,"Root inventory unavailable");auto value=parse(response.output);
    require(value["blockdevices"].isArray(),"Malformed root inventory");value["schema"]="org.uke.control.roots.v1";value["mounted_by_command"]=false;value["target_code_executed"]=false;return value;
}
void candidates(const Value& rows,const std::string& id,std::vector<Value>& matches,unsigned depth=0) {
    require(depth<=16&&rows.isArray()&&rows.size()<=256,"Root inventory exceeds its bound");
    for(const auto& row:rows) {
        if(row["type"]=="part"&&row["partuuid"]==id)matches.push_back(row);
        if(row.isMember("children"))candidates(row["children"],id,matches,depth+1);
    }
}
std::string inside(int root,const char* path,size_t limit,bool prefix=false) {
    open_how how{};how.flags=O_RDONLY|O_CLOEXEC|O_NONBLOCK;how.resolve=RESOLVE_IN_ROOT|RESOLVE_NO_MAGICLINKS;
    Descriptor fd{int(syscall(SYS_openat2,root,path,&how,sizeof(how)))};
    if(fd.fd<0)return {};
    struct stat st{};require(!fstat(fd.fd,&st)&&S_ISREG(st.st_mode),"Inspected root metadata must be a regular file");
    if(prefix) {std::array<char,64> data{};const auto n=::read(fd.fd,data.data(),data.size());require(n>=0,"Cannot read root executable header");return {data.data(),size_t(n)};}
    return bytes(fd.fd,limit);
}
Value inspect_root(const std::string& id) {
    require(id.size()<=64&&!id.empty()&&id.find_first_not_of("0123456789abcdefABCDEF-")==std::string::npos,"Use an exact partition UUID from roots list");
    auto value=roots();std::vector<Value> matches;candidates(value["blockdevices"],id,matches);
    require(matches.size()==1,"Root identity is missing or ambiguous");const auto& target=matches.front();
    value=target;value["schema"]="org.uke.control.root-inspection.v1";value["mounted_by_command"]=false;value["target_code_executed"]=false;
    std::vector<std::string> mounts;for(const auto& path:target["mountpoints"])if(path.isString()&&!path.asString().empty())mounts.push_back(path.asString());
    if(mounts.empty()){value["inspection"]="not_mounted";return value;}
    require(mounts.size()==1,"Select a root with exactly one current mount");
    Descriptor root{open(mounts.front().c_str(),O_PATH|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC)};struct stat st{},current{};
    require(root.fd>=0&&!fstat(root.fd,&st)&&!stat("/",&current),"Cannot verify the mounted root");
    require(target["maj:min"]==std::to_string(major(st.st_dev))+":"+std::to_string(minor(st.st_dev)),"Mounted root identity changed");
    value["same_as_current_root"]=st.st_dev==current.st_dev;
    auto release=inside(root.fd,"etc/os-release",8192);if(release.empty())release=inside(root.fd,"usr/lib/os-release",8192);
    value["os_release"]=parse(sysfs::quote(release));value["inspection"]="mounted_read_only_observation";
    const auto shell=inside(root.fd,"bin/sh",64,true);
    value["shell_architecture"]=shell.size()>20&&shell.compare(0,4,"\177ELF")==0&&shell[4]==2&&shell[5]==1?
        (uint8_t(shell[18])==183&&shell[19]==0?"aarch64":uint8_t(shell[18])==62&&shell[19]==0?"x86_64":"other_elf"):"unknown";
    return value;
}
}
int command(int argc,char** argv,bool as_json) {
    const std::string action=argv[0];Value output;
    if(action=="capabilities"&&argc==1) {
        output["schema"]="org.uke.control.capabilities.v1";output["application_version"]=cli::version;
        output["physical_acceptance"]=Json::nullValue;output["read_only_components"]=Value(Json::arrayValue);
        for(const auto& domain:domains)output["read_only_components"].append(domain);
        output["commands"]=Value(Json::arrayValue);for(const auto& name:cli::command_names())output["commands"].append(name);
        output["metadata_only"]=true;output["hardware_controls_qualified"]=Json::nullValue;
    } else if(action=="complete"&&(argc==1||argc==2)) {
        const std::string prefix=argc==2?argv[1]:"";auto names=prefix.starts_with('-')?std::vector<std::string>{"--help","--json","--version"}:cli::command_names();
        output["schema"]="org.uke.control.completions.v1";output["candidates"]=Value(Json::arrayValue);
        for(const auto& name:names)if(name.starts_with(prefix)){output["candidates"].append(name);if(!as_json)std::cout<<name<<'\n';}
        if(!as_json)return 0;
    } else if(action=="overview"&&argc==3&&std::string_view(argv[1])=="--fixture") {
        output=load(argv[2]);validate(output);output["simulation"]=true;output["physical_acceptance"]=Json::nullValue;
    } else if(action=="overview"&&(argc==1||argc==2))output=overview(argc==2?argv[1]:"");
    else if(action=="overview"&&(argc==3||argc==4)&&std::string_view(argv[1])=="save") {
        output=overview(argc==4?argv[3]:"");save(argv[2],output);output=Value(Json::objectValue);output["saved"]=true;output["mode"]="0600";
    } else if(action=="compare"&&argc==3)output=compare(load(argv[1]),load(argv[2]));
    else if(action=="support"&&argc==4&&std::string_view(argv[1])=="export") {
        output=exported(load(argv[2]));save(argv[3],output);
    } else if(action=="roots"&&argc==2&&std::string_view(argv[1])=="list")output=roots();
    else if(action=="roots"&&argc==3&&std::string_view(argv[1])=="inspect")output=inspect_root(argv[2]);
    else throw cli::UsageError("Invalid overview, capabilities, compare, support or roots arguments; see --help");
    cli::print(encode(output),as_json);return 0;
}
}
