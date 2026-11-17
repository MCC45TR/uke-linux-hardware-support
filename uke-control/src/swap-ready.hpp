// SPDX-License-Identifier: MIT
#pragma once
#include <algorithm>
#include <string>
#include <vector>
#include <cstdint>

namespace uke::swap_startup {
struct State {
    bool admission_completed{}, trusted_helper{}, block{}, managed_file_active{};
    std::string mapper_uuid, mapper_info, signature;
    unsigned major{}, minor{};
    uint64_t capacity{}, page_size{}, active_bytes{};
};
inline bool reusable(const State& state) {
    // Only the existing logical linux_swap fast path is reusable. Other cases
    // run full discovery, three-slot admission and partition/file selection.
    const std::string prefix="UKE-SWAP-v1-";
    if (!state.admission_completed || !state.trusted_helper || !state.block || state.managed_file_active ||
        !state.mapper_uuid.starts_with(prefix) || state.mapper_uuid.size()>96 ||
        state.page_size!=4096 || state.capacity<=state.page_size ||
        state.capacity%state.page_size || state.active_bytes!=state.capacity-state.page_size) return false;
    const auto backing=state.mapper_uuid.substr(prefix.size());
    const auto separator=backing.find('-');
    if (separator==std::string::npos || separator==0 || separator+1==backing.size() ||
        backing.find('-',separator+1)!=std::string::npos ||
        backing.find_first_not_of("0123456789-")!=std::string::npos) return false;
    // A nonzero DM event count means the mapping may have changed after initial
    // admission. It must return through the complete verifier, never this path.
    const auto expected=state.mapper_uuid+":"+std::to_string(state.major)+":"+
                        std::to_string(state.minor)+":0\n";
    return state.mapper_info==expected && state.signature=="swap\nlinux_swap\n";
}
} // namespace uke::swap_startup
