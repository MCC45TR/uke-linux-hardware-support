// SPDX-License-Identifier: MIT
#pragma once
#include <string_view>
namespace uke::setup_cdc {
enum class Request { help, status, logs, bootloader, invalid };
inline Request request(std::string_view line) {
    if(line=="help")return Request::help;
    if(line=="status")return Request::status;
    if(line=="logs")return Request::logs;
    if(line=="reboot bootloader")return Request::bootloader;
    return Request::invalid;
}
int command(int argc,char** argv);
}
