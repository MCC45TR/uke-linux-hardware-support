// SPDX-License-Identifier: MIT
#pragma once
#include <string>
#include <string_view>
#include <stdexcept>

enum class RollbackMode { enforced, permissive, off };
inline RollbackMode rollback_mode(std::string_view value) {
    if(value=="enforced")return RollbackMode::enforced;
    if(value=="permissive")return RollbackMode::permissive;
    if(value=="off")return RollbackMode::off;
    throw std::runtime_error("Expected rollback mode enforced, permissive or off");
}
inline std::string rollback_name(RollbackMode mode) {
    switch(mode) {
    case RollbackMode::enforced:return "enforced";
    case RollbackMode::permissive:return "permissive";
    case RollbackMode::off:return "off";
    }
    throw std::runtime_error("Invalid rollback mode");
}
inline bool rollback_retry(RollbackMode mode,bool explicit_request) {
    return explicit_request && mode!=RollbackMode::enforced;
}
static RollbackMode read_rollback_policy(const fs::path &path="/etc/senemos/rollback.conf") {
    if(!policy_exists(path))return RollbackMode::enforced;
    const auto value=policy_text(path);
    for(const auto mode:{RollbackMode::enforced,RollbackMode::permissive,RollbackMode::off})
        if(value=="mode="+rollback_name(mode)+"\n")return mode;
    throw std::runtime_error("Invalid rollback policy; expected canonical mode=enforced, permissive or off");
}
static void set_rollback_policy(RollbackMode mode,const fs::path &path="/etc/senemos/rollback.conf") {
    auto dir=directory(path.parent_path());
    Fd lock(openat(dir.value,"rollback.lock",O_RDWR|O_CREAT|O_NOFOLLOW|O_CLOEXEC,0600));trusted_fd(lock.value);
    need(!flock(lock.value,LOCK_EX|LOCK_NB),"Another rollback policy change is active");
    const auto before=policy_exists(path)?policy_text(path):std::string{};
    (void)read_rollback_policy(path);
    replace_policy(path,before,"mode="+rollback_name(mode)+"\n");
    need(read_rollback_policy(path)==mode,"Rollback policy readback differs");
}
