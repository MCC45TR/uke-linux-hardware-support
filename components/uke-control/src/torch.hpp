// SPDX-License-Identifier: MIT
#pragma once
#include "lights.hpp"
namespace uke::torch {
inline constexpr const char* bus_name="org.senemos.Uke.Torch";
inline constexpr const char* object_path="/org/senemos/Uke/Torch";
inline constexpr const char* interface="org.senemos.Uke.Torch1";
inline constexpr const char* action="org.senemos.uke.torch.control";
class Actuator {
    int lock_fd=-1,brightness_fd=-1;
    lights::Source source;
    unsigned requested=100;
    void apply(unsigned level);
public:
    Actuator();
    ~Actuator();
    Actuator(const Actuator&)=delete;
    Actuator& operator=(const Actuator&)=delete;
    void set(unsigned percent);
    void enable(bool on);
    std::string status();
};
int command(int argc,char** argv,bool as_json=false);
int prepare_access(bool remove=false);
}
