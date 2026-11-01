// SPDX-License-Identifier: MIT
#pragma once
#include <array>
#include <functional>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace uke::packages::operation {
enum class Resume {replan,finish_boot,finish_selection,review};
struct Record {std::string id,channel,phase,kernel;bool boot_required=false;};
inline Resume resume(const std::string& phase) {
    if(phase=="prepared"||phase=="repositories")return Resume::replan;
    if(phase=="rpm_complete"||phase=="boot_started")return Resume::finish_boot;
    if(phase=="boot_complete"||phase=="selected"||phase=="complete")return Resume::finish_selection;
    return Resume::review;
}
inline const char* next(const std::string& phase) {
    switch(resume(phase)) {
    case Resume::replan:return "review_new_plan";
    case Resume::finish_boot:return "resume_boot_finalization";
    case Resume::finish_selection:return "resume_selection_finalization";
    case Resume::review:return "inspect_rpm_history_no_automatic_retry";
    }
    return "inspect_rpm_history_no_automatic_retry";
}
inline void validate(const Record& record) {
    if(record.id.size()!=32||record.id.find_first_not_of("0123456789abcdef")!=std::string::npos||
       (record.channel!="stable"&&record.channel!="test"&&record.channel!="unstable")||
       record.kernel.size()>256||record.kernel.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._+~-")!=std::string::npos||
       (record.boot_required&&record.kernel.empty()))throw std::runtime_error("Invalid package operation identity");
    bool phase=false;for(const auto* known:{"prepared","repositories","rpm_started","rpm_complete","boot_started","boot_complete","selected","complete"})phase|=record.phase==known;
    if(!phase)throw std::runtime_error("Unknown package operation phase");
}
inline std::string encode(const Record& record) {
    validate(record);return "format=1\nid="+record.id+"\nchannel="+record.channel+"\nphase="+record.phase+"\nkernel="+record.kernel+"\nboot_required="+(record.boot_required?"1":"0")+"\n";
}
inline Record decode(const std::string& text) {
    if(text.size()>512||!text.ends_with('\n'))throw std::runtime_error("Incomplete package operation record");
    std::map<std::string,std::string> fields;std::istringstream lines(text);std::string line;
    while(std::getline(lines,line)) {
        const auto split=line.find('=');
        if(split==line.npos||!fields.emplace(line.substr(0,split),line.substr(split+1)).second)throw std::runtime_error("Malformed package operation record");
    }
    if(fields.size()!=6||fields.at("format")!="1"||(fields.at("boot_required")!="0"&&fields.at("boot_required")!="1"))throw std::runtime_error("Unsupported package operation record");
    Record record{fields.at("id"),fields.at("channel"),fields.at("phase"),fields.at("kernel"),fields.at("boot_required")=="1"};validate(record);return record;
}
struct Actions {
    std::function<void(const Record&)> checkpoint;
    std::function<void()> repositories,rpm,boot,select,clear;
};
inline void finish(Record& record,const Actions& actions) {
    const auto policy=resume(record.phase);
    if(policy==Resume::review||policy==Resume::replan)throw std::runtime_error("RPM completion is not verified; finalization is refused");
    if(policy==Resume::finish_boot) {
        record.phase="boot_started";actions.checkpoint(record);
        if(record.boot_required)actions.boot();
        record.phase="boot_complete";actions.checkpoint(record);
    }
    if(record.phase=="boot_complete") {
        actions.select();record.phase="selected";actions.checkpoint(record);
    }
    if(record.phase!="complete"){record.phase="complete";actions.checkpoint(record);}
    actions.clear();
}
inline void apply(Record& record,const Actions& actions) {
    if(resume(record.phase)!=Resume::replan)throw std::runtime_error("Package mutation cannot replay a started RPM operation");
    record.phase="repositories";actions.checkpoint(record);actions.repositories();
    record.phase="rpm_started";actions.checkpoint(record);actions.rpm();
    record.phase="rpm_complete";actions.checkpoint(record);
    finish(record,actions);
}
}
