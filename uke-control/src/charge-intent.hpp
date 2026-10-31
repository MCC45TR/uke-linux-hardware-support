// SPDX-License-Identifier: MIT
#pragma once
#include <stdexcept>
#include <string>
#include <string_view>
namespace uke::charge::intent {
inline std::string standard(unsigned percent) {
    if(percent<1||percent>100)throw std::runtime_error("Invalid legacy charge intent");
    return "format=1\nprovider=standard_threshold\nrequested="+std::to_string(percent)+"\n";
}
inline std::string qualified(std::string_view mode) {
    for(const auto* known:{"charge80","hold70-80","hold80-90","slow","automatic","profile-restore","enable-cap80","apply-cap80"})
        if(mode==known)return "format=1\nprovider=qualified\nrequested="+std::string(mode)+"\n";
    throw std::runtime_error("Unsupported qualified charge intent");
}
inline bool restore_legacy(const std::string& record,unsigned percent) {
    if(record.empty()||record==standard(percent))return true;
    for(const auto* mode:{"charge80","hold70-80","hold80-90","slow","automatic","profile-restore","enable-cap80","apply-cap80"})if(record==qualified(mode))return false;
    throw std::runtime_error("Saved charge intent is unknown or differs from the legacy limit; restoration refused");
}
}
