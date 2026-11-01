// SPDX-License-Identifier: MIT
#pragma once
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <utility>

namespace uke::reporting {
class Budget {
    std::filesystem::path path_;
    size_t maximum_,used_=0;
    uintmax_t reserve_;
    std::chrono::steady_clock::time_point deadline_;
public:
    explicit Budget(std::filesystem::path path,size_t maximum=63*1024*1024,unsigned seconds=300,uintmax_t reserve=256*1024*1024):
        path_(std::move(path)),maximum_(maximum),reserve_(reserve),deadline_(std::chrono::steady_clock::now()+std::chrono::seconds(seconds)){}
    unsigned seconds() const {
        const auto left=std::chrono::duration_cast<std::chrono::seconds>(deadline_-std::chrono::steady_clock::now()).count();
        return left>0?unsigned(left):0;
    }
    size_t allowance(size_t requested) const {
        if(!seconds())return 0;
        const auto available=std::filesystem::space(path_).available;
        if(available<=reserve_)return 0;
        return std::min({requested,maximum_-used_,size_t(std::min<uintmax_t>(available-reserve_,SIZE_MAX))});
    }
    void charge(size_t bytes) {
        if(bytes>maximum_-used_)throw std::runtime_error("Report exceeds its total byte budget");
        used_+=bytes;
    }
    size_t used() const{return used_;}
};
}
