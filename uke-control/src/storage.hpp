// SPDX-License-Identifier: MIT
#pragma once
#include <filesystem>
#include <string>
namespace uke::storage {
// Read-only observations. Never an authorization token for a later disk operation.
std::string snapshot(const std::filesystem::path& root, const std::string& selection={});
}
namespace uke::storage {
std::string plan(const std::filesystem::path& root,const std::string& device,const std::string& operation,const std::string& size={});
}
