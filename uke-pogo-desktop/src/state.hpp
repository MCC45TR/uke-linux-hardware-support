// SPDX-License-Identifier: MIT
#pragma once
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
namespace uke_pogo_desktop {
namespace fs=std::filesystem;
inline constexpr auto led_name="uke-pogo::kbd_backlight";
inline constexpr auto of_path="firmware/devicetree/base/soc/qcom,qupv3_0_geni_se@ac0000/i2c@a8c000/nanosic@4c";
template<class Read> bool present(const fs::path &sys,Read read) {
    std::error_code error;
    const auto led=fs::canonical(sys/"class/leds"/led_name,error);
    if(error)return false;
    const auto client=fs::canonical(led/"device",error);
    if(error)return false;
    const auto node=fs::canonical(client/"of_node",error);
    if(error||node!=sys/of_path)return false;
    const auto driver=fs::canonical(client/"driver",error);
    if(error||driver!=sys/"bus/i2c/drivers/hid-nanosic-uke")return false;
    if(led!=client/"leds"/led_name)return false;
    return read(led/"max_brightness")=="100\n"&&read(client/"bridge_health").starts_with("attached=1\n");
}
inline bool compatible(std::string_view bytes,std::string_view identity) {
    size_t offset=0;
    while(offset<bytes.size()) {
        const auto end=bytes.find('\0',offset);
        if(end==std::string_view::npos)return false;
        if(bytes.substr(offset,end-offset)==identity)return true;
        offset=end+1;
    }
    return false;
}
struct Observation {
    bool pogo=false,upower_ready=false,powerdevil_running=false,powerdevil_supported=false;
    int upower_max=0,powerdevil_max=0;
};
class Refresh {
    std::optional<bool> previous_;
    bool managed_=false,attempted_=false;
public:
    bool decide(const Observation &state) {
        if(!previous_||*previous_!=state.pogo) {
            previous_=state.pogo;attempted_=false;
            managed_=managed_||state.pogo;
        }
        if(!managed_||attempted_||!state.upower_ready||!state.powerdevil_running)return false;
        const bool supported=state.upower_max>0;
        const bool stale=state.powerdevil_supported!=supported||
            (supported&&state.powerdevil_max!=state.upower_max);
        if(!stale) {
            if(!state.pogo)managed_=false;
            return false;
        }
        attempted_=true;
        return true;
    }
};
}
