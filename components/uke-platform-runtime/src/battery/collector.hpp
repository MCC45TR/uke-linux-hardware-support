// SPDX-License-Identifier: MIT
#pragma once
#include <string>
#include "session.hpp"
namespace uke::battery {
std::string collect(const std::string &sysfs_root, ModelSession &models, bool inventory_only = false);
std::string capabilities();
std::string quote(const std::string &value);
int serve();
}
