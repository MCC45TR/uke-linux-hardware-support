// SPDX-License-Identifier: MIT
#pragma once
#include "trial-health.hpp"
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

class CriticalHealthFailure : public TrialHealthFailure {
public:
    using TrialHealthFailure::TrialHealthFailure;
};

inline std::string critical_health_value(const std::filesystem::path &path) {
    std::ifstream input(path);
    std::string value;
    if(!input || !std::getline(input,value) || value.size()>128)return {};
    // Sysfs attributes have exactly one short value, terminated by a newline.
    std::string extra;
    if(std::getline(input,extra))return {};
    return value;
}

// This is an observation only. It cannot select a slot, approve a kernel,
// restart firmware or weaken the updater's image and metadata checks.
inline void critical_platform_health(const std::filesystem::path &root="/") {
    const auto remote=root/"sys/class/remoteproc";
    std::error_code error;
    if(!std::filesystem::is_directory(remote,error))return;
    for(const auto &entry:std::filesystem::directory_iterator(remote)) {
        if(critical_health_value(entry.path()/"name")!="adsp")continue;
        const auto state=critical_health_value(entry.path()/"state");
        if(state=="crashed")
            throw CriticalHealthFailure("critical: ADSP remote processor crashed; audio and battery telemetry may be unavailable");
        // Recovery/offline transitions are not a confirmed permanent crash.
        // They may fail normal health qualification without an immediate reboot.
        if(state!="running")throw std::runtime_error("ADSP is not running: "+state);
    }
}

inline bool critical_fallback_policy(std::string_view mode,bool critical) {
    if(mode=="enforced")return true;
    if(mode=="permissive")return critical;
    if(mode=="off")return false;
    throw std::runtime_error("Invalid rollback mode for critical fallback");
}

// A runtime notification is not a polling tick. A busy transaction defers to
// its state-file notification; off logs once without invoking the writer.
template<class Probe,class Policy,class Record,class Emit>
void critical_runtime_event(Probe probe,Policy policy,Record record,Emit emit,bool &off_reported) {
    try {probe();}
    catch(const CriticalHealthFailure &failure) {
        const auto mode=policy();
        if(!critical_fallback_policy(mode,true)) {
            if(!off_reported)emit(mode,failure.what(),false);
            off_reported=true;return;
        }
        if(!record())return;
        emit(mode,failure.what(),true);
        throw;
    }catch(const std::exception &) {return;}
}

// Bind a critical rejection to this exact boot, kernel and protected slot
// state. A stale or malformed marker never authorizes a permissive fallback.
inline bool critical_rejection_matches(std::string_view marker,
        std::string_view release,std::string_view boot,std::string_view state_hash) {
    if(release.empty() || release.size()>128 || release.find_first_of("\r\n=")!=release.npos ||
       boot.size()!=36 || state_hash.size()!=64)return false;
    for(size_t i=0;i<boot.size();i++) {
        if(i==8 || i==13 || i==18 || i==23) {if(boot[i]!='-')return false;}
        else if(!((boot[i]>='0' && boot[i]<='9') || (boot[i]>='a' && boot[i]<='f')))return false;
    }
    for(const char value:state_hash)
        if(!((value>='0' && value<='9') || (value>='a' && value<='f')))return false;
    return marker=="format=1\nkernel="+std::string(release)+"\nboot="+std::string(boot)+
        "\nstate_sha256="+std::string(state_hash)+"\ncritical=adsp-crashed\n";
}
