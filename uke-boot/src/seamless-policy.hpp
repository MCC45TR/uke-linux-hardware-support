// SPDX-License-Identifier: MIT
#pragma once
#include <charconv>

inline std::map<std::string,std::string> seamless_values(const std::string &text) {
    std::istringstream input(text);std::string row,filtered;
    while(std::getline(input,row))if(!row.empty() && row.front()!='#')filtered+=row+'\n';
    return properties(filtered);
}

// Installation policy and the user's rollback choice have separate owners.
// This migration never reads or writes rollback.conf.
inline bool enable_seamless_policy(const fs::path &configuration,const fs::path &state) {
    auto config=directory(configuration);auto saved=directory(state,true);
    Fd lock(openat(saved.value,"lock",O_RDWR|O_CREAT|O_NOFOLLOW|O_CLOEXEC,0600));trusted_fd(lock.value);
    need(!flock(lock.value,LOCK_EX|LOCK_NB),"Another kernel update or policy migration is active");
    const auto global=configuration/"kernel-update.conf",profile=configuration/"userdata-update.conf";
    const auto before_global=policy_text(global),before_profile=policy_text(profile);
    const auto g=seamless_values(before_global),p=properties(before_profile);
    need(g.size()==1 && g.contains("automatic") && (g.at("automatic")=="0" || g.at("automatic")=="1"),
         "Unrecognized global seamless policy");
    need((p.size()==1 || p.size()==2) && p.contains("automatic") &&
         (p.at("automatic")=="0" || p.at("automatic")=="1") &&
         (p.size()==1 || (p.contains("vrr_min_hz") && (p.at("vrr_min_hz")=="0" || p.at("vrr_min_hz")=="30"))),
         "Unrecognized userdata seamless policy");
    const auto next_profile=p.at("automatic")=="1"?before_profile:
        "automatic=1\nvrr_min_hz="+(p.contains("vrr_min_hz")?p.at("vrr_min_hz"):std::string("0"))+"\n";
    replace_policy(profile,before_profile,next_profile);
    const auto next_global=g.at("automatic")=="1"?before_global:std::string("automatic=1\n");
    replace_policy(global,before_global,next_global);
    return before_global!=next_global || before_profile!=next_profile;
}

enum class SeamlessStage {current,apply,replace_trial,pending,older,deferred};
inline std::array<uint64_t,4> unstable_version(const std::string &release) {
    need(release.ends_with("-unstable"),"Automatic development updates require the unstable ABI");
    const auto text=release.substr(0,release.size()-9);std::array<uint64_t,4> result{};
    size_t begin=0;
    for(size_t i=0;i<result.size();++i) {
        const auto end=i==3?text.size():text.find(i==2?'-':'.',begin);
        need(end!=std::string::npos && end>begin,"Malformed development kernel version");
        const auto parsed=std::from_chars(text.data()+begin,text.data()+end,result[i]);
        need(parsed.ec==std::errc{} && parsed.ptr==text.data()+end,"Malformed development kernel number");
        begin=end+1;
    }
    return result;
}
inline bool pending_development_reboot(RollbackMode mode,const uke_install::Update &saved,
        const std::string &staged_boot_id,const std::string &boot_id,const std::string &running) {
    return mode==RollbackMode::off && saved.phase==uke_install::Phase::trial &&
        saved.good==uke_install::Slot::b && !staged_boot_id.empty() && staged_boot_id==boot_id &&
        !saved.candidate.empty() && saved.candidate!=running;
}
inline SeamlessStage seamless_stage(RollbackMode mode,const uke_install::Update &saved,
        const std::string &staged_boot_id,const std::string &boot_id,const std::string &running,const std::string &selected) {
    if(pending_development_reboot(mode,saved,staged_boot_id,boot_id,running))return SeamlessStage::pending;
    if(selected==running)return SeamlessStage::current;
    need(selected!=saved.failed,"Previously failed automatic candidate refused");
    if(mode!=RollbackMode::off)return SeamlessStage::apply;
    if(running.ends_with("-unstable") && selected.ends_with("-unstable") &&
       unstable_version(selected)<unstable_version(running))return SeamlessStage::older;
    if(saved.phase==uke_install::Phase::stable)return SeamlessStage::apply;
    if(saved.phase==uke_install::Phase::trial && saved.good==uke_install::Slot::b &&
       saved.candidate==running && !boot_id.empty() && staged_boot_id!=boot_id)return SeamlessStage::replace_trial;
    return SeamlessStage::deferred;
}
inline bool skip_off_acceptance(RollbackMode mode,bool matching) {
    if(mode!=RollbackMode::off)return false;
    need(matching,"Off-mode boot does not match the protected baseline or trial");return true;
}
