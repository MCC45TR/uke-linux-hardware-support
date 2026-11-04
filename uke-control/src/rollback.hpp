// SPDX-License-Identifier: MIT
#pragma once
#include "cli.hpp"
#include <string>
#include <vector>
namespace uke::rollback {
inline std::vector<std::string> plan(const std::vector<std::string> &words) {
    const std::vector<std::string> status={"/usr/bin/uke-kernel-update","rollback-policy","status"};
    if(words==std::vector<std::string>{"rollback","status"})return status;
    if(words.size()==2 && words[0]=="set") {
        for(const std::string mode:{"enforced","permissive","off"})
            if(words[1]=="rollback-"+mode)return {status[0],status[1],"set",mode};
    }
    throw cli::UsageError("use rollback status or set rollback-enforced|rollback-permissive|rollback-off");
}
}
