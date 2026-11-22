// SPDX-License-Identifier: MIT
#pragma once
#include <stdexcept>
#include <unistd.h>
namespace uke::update_status {
inline void command(bool json) {
    // Only this fixed, argument-free read operation is authorized by PolicyKit.
    constexpr const char *helper="/usr/libexec/uke-control/update-status";
    const char *root[]={helper,json?"--json":nullptr,nullptr};
    const char *session[]={"/usr/bin/pkexec",helper,json?"--json":nullptr,nullptr};
    const auto arguments=geteuid()==0?root:session;
    execv(arguments[0],const_cast<char *const *>(arguments));
    throw std::runtime_error("The update-status helper is unavailable; install the matched uke-control package");
}
inline void select_slot(const std::string &target,bool preview) {
    const char *arguments[]={"/usr/libexec/uke-control/set-slot",target.c_str(),preview?"--dry-run":nullptr,nullptr};
    execv(arguments[0],const_cast<char *const *>(arguments));
    throw std::runtime_error("The set-slot helper is unavailable; install the matched uke-control package");
}
}
