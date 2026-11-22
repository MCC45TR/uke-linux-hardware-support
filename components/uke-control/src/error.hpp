// SPDX-License-Identifier: MIT
#pragma once
#include "sysfs.hpp"
#include <iostream>
#include <string_view>
namespace uke::cli {
inline bool json_requested_inline(int argc,char** argv) {
    for(int i=1;i<argc;++i){const std::string_view word=argv[i];if(word=="--")break;if(word=="--json")return true;}
    return false;
}
inline int json_failure(const std::string& message,int status) {
    std::cerr<<"{\"schema\":\"org.uke.control.error.v1\",\"error\":{\"code\":\""<<(status==64?"USAGE_ERROR":"OPERATION_FAILED")
        <<"\",\"message\":"<<sysfs::quote(message)<<",\"retryable\":false,\"operation_id\":null},\"exit_status\":"<<status<<"}\n";
    return status;
}
}
