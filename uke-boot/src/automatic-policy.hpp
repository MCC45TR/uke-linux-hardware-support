// SPDX-License-Identifier: MIT
#pragma once

// Uses the updater's descriptor, owner, bounded-read and durable-write helpers.
static bool policy_exists(const fs::path &path) {
    struct stat st{};
    if(!lstat(path.c_str(),&st))return true;
    need(errno==ENOENT,"Cannot inspect automatic policy migration");return false;
}
static std::string policy_text(const fs::path &path) {
    auto fd=regular(path);struct stat st{};
    need(!fstat(fd.value,&st) && st.st_size>0 && st.st_size<=4096,"Automatic policy exceeds bounded text size");
    const auto bytes=read_fd(fd.value,st.st_size);return {bytes.begin(),bytes.end()};
}
static void replace_policy(const fs::path &path,const std::string &expected,const std::string &value) {
    auto dir=directory(path.parent_path());mode_t mode=0600;
    const bool exists=policy_exists(path);
    if(exists) {
        auto previous=regular(path);struct stat st{};need(!fstat(previous.value,&st),"Cannot stat automatic policy");
        mode=st.st_mode&0777;
        const auto current=policy_text(path);if(current==value)return;
        need(current==expected,"Automatic policy changed during migration");
    } else need(expected.empty(),"Required automatic policy disappeared");
    const auto temporary=path.filename().string()+".automatic-policy.new";
    if(policy_exists(path.parent_path()/temporary)) {
        need(policy_text(path.parent_path()/temporary)==value,"Unrecognized pending automatic policy write");
        need(!unlinkat(dir.value,temporary.c_str(),0),"Cannot retire pending automatic policy write");
    }
    Fd next(openat(dir.value,temporary.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600));
    trusted_fd(next.value);need(!fchmod(next.value,mode),"Cannot preserve automatic policy permissions");
    write_fd(next.value,Bytes(value.begin(),value.end()));
    need(exists ? policy_text(path)==expected : !policy_exists(path),"Automatic policy changed before commit");
    need(!renameat(dir.value,temporary.c_str(),dir.value,path.filename().c_str()) && !fsync(dir.value),"Cannot commit automatic policy migration");
    need(policy_text(path)==value,"Automatic policy readback differs");
}
static const char policy_prepared[]="format=1\nphase=prepared\n";
static const char policy_complete[]="format=1\nphase=complete\n";
static const char policy_retained[]="format=1\nphase=retained\n";
static bool automatic_policy_ready(const fs::path &state) {
    const auto marker=state/"automatic-policy-v1";
    if(!policy_exists(marker))return true;
    const auto phase=policy_text(marker);
    need(phase==policy_prepared || phase==policy_complete || phase==policy_retained,"Unknown automatic policy migration state");
    return phase!=policy_prepared;
}
template<class Commit=decltype(&replace_policy)>
static bool migrate_automatic_policy(const fs::path &configuration,const fs::path &state,Commit commit=replace_policy) {
    auto config=directory(configuration);auto saved=directory(state,true);
    Fd lock(openat(saved.value,"lock",O_RDWR|O_CREAT|O_NOFOLLOW|O_CLOEXEC,0600));trusted_fd(lock.value);
    need(!flock(lock.value,LOCK_EX|LOCK_NB),"Another kernel update or policy migration is active");
    const auto marker=state/"automatic-policy-v1";
    const auto global=configuration/"kernel-update.conf",profile=configuration/"userdata-update.conf";
    const std::string old="automatic=0\n",enabled="automatic=1\n",userdata="automatic=1\nvrr_min_hz=0\n";
    bool resume=false;
    if(policy_exists(marker)) {
        if(automatic_policy_ready(state))return false;
        resume=true;
    }
    const auto before_global=policy_text(global),before_profile=policy_text(profile);
    if(!resume && (before_global!=old || before_profile!=old)) {
        commit(marker,"",policy_retained);return false;
    }
    need((before_global==old || before_global==enabled) && (before_profile==old || before_profile==userdata),
         "Legacy automatic policy changed during recovery");
    for(const auto name:{"global-policy.before-v1","userdata-policy.before-v1"}) {
        const auto backup=state/name;
        if(policy_exists(backup))need(policy_text(backup)==old,"Automatic policy backup differs");
        else {need(!resume,"Automatic policy recovery backup missing");commit(backup,"",old);}
    }
    if(!resume)commit(marker,"",policy_prepared);
    commit(profile,before_profile,userdata);
    commit(global,before_global,enabled);
    commit(marker,policy_prepared,policy_complete);
    return true;
}
