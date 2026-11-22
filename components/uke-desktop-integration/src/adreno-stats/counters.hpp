// SPDX-License-Identifier: MIT
#pragma once
#include <algorithm>
#include <charconv>
#include <cstdint>
#include <optional>
#include <string_view>
namespace uke {
inline std::optional<std::uint64_t> unsigned_value(std::string_view text) {
    constexpr std::string_view spaces=" \t\r\n";
    auto start=text.find_first_not_of(spaces);
    if(start==text.npos)return std::nullopt;
    text.remove_prefix(start);text=text.substr(0,text.find_last_not_of(spaces)+1);
    std::uint64_t value{};
    auto [end,error]=std::from_chars(text.data(),text.data()+text.size(),value);
    if(error!=std::errc{}||end!=text.data()+text.size())return std::nullopt;
    return value;
}
class Activity {
    std::optional<std::uint64_t> previous_;
    std::uint64_t time_{};
public:
    void reset(){previous_.reset();}
    std::optional<double> sample(std::uint64_t counter,std::uint64_t time) {
        auto previous=previous_;auto before=time_;
        previous_=counter;time_=time;
        if(!previous||counter<*previous||time<=before)return std::nullopt;
        return std::clamp(100.0*double(counter-*previous)/double(time-before),0.0,100.0);
    }
};
}
