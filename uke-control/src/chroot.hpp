// SPDX-License-Identifier: MIT
#pragma once
#include <filesystem>
#include <string>
#include <vector>
namespace uke::chroot_manager {
struct Target {std::string path, identity, filesystem, uuid, system, token;};
std::string system_id(const std::string& release);
std::string no_recovery_options(const std::string& type);
std::string target_release(int root_fd);
void require_native_shell(int root_fd);
std::string fingerprint(const Target& target,const std::string& release);
std::vector<std::string> candidates(const std::string& device);
Target inspect(const std::string& device);
int enter(const std::string& device,const std::string& expected_token);
std::string scan(const std::string& device);
}
